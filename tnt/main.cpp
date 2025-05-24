#include "piece_storage/piece_storage.h"
#include "torrent_file/parser.h"
#include "torrent_file/types.h"
#include "torrent_tracker.h"
#include "peer_connection/peer_connection.h"
#include "download_manager/download_manager.h"
#include <cxxopts.hpp>
#include <indicators/multi_progress.hpp>
#include <indicators/progress_bar.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <ostream>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;
using namespace indicators;
namespace fs = std::filesystem;

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);

    cxxopts::Options options("tnt", "tnt is a simple torrent client");
    options.add_options()
        ("h,help", "Print usage")
        ("o,out", "Output file name.", cxxopts::value<std::string>())
        ("file", "Metainfo (.torrent) file.", cxxopts::value<std::string>());

    options.parse_positional({"file"});
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        options.positional_help("<torrent file>");
        std::cout << options.help() << std::endl;
        return 0;
    }

    fs::path tfPath = result["file"].as<std::string>();
    fs::path outputFilePath = result["out"].as<std::string>();
    
    TorrentFile file;
    std::ifstream stream(tfPath);
    stream >> file;

    std::cout << "loading torrent file..." << std::endl;
    std::cout << " - name: " << file.name << std::endl;
    std::cout << " - announce: " << file.announce << std::endl;
    std::cout << " - description: " << file.comment << std::endl;
    // std::cout << " - piece size: " << file.pieceLength << std::endl;
    // std::cout << " - pieces: " << file.pieceHashes.size() << std::endl << std::endl;

    std::cout << "getting peers..." << std::flush; 
    TorrentTracker tracker(file.announce);
    tracker.UpdatePeers(file, "TEST0APP1DONT2WORRY3", 12345);
    auto& peers = tracker.GetPeers();
    std::cout << " found " << peers.size() << " peers" << std::endl;

    std::ofstream outputFile(outputFilePath);
    PieceStorage pieceStorage(file, outputFile);

    std::mutex mtx;
    std::vector<std::thread> threads;
    std::vector<DownloadManager*> mngs;
    
    ProgressBar bar1{
        option::BarWidth{50},
        option::Start{"["},
        option::Fill{"#"},
        option::Lead{"#"},
        option::Remainder{" "},
        option::End{"]"},
        option::ForegroundColor{Color::green},
        option::ShowPercentage{true},
        option::ShowElapsedTime{true},
        option::ShowRemainingTime{true},
        option::PrefixText{"downloading "},
        option::FontStyles{std::vector<FontStyle>{FontStyle::bold}}
    };
    
    indicators::MultiProgress<indicators::ProgressBar, 1> bars(bar1);

    auto progressBarJob = [&]() {
        while (true) {
            bars.set_progress<0>(pieceStorage.GetDownloadedCount() * 100 / file.pieceHashes.size());
            if (bars.is_completed<0>())
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };
    
    threads.emplace_back(progressBarJob);

    std::atomic<int> cnt = 0;

    // very shitty, i'll fix that, i promise
    for (auto peer : peers) {
        if (pieceStorage.AllPiecesDownloaded())
            break;
        threads.emplace_back([&]() {
            int sec = 2;
            while (!pieceStorage.AllPiecesDownloaded()) {
                auto mng = DownloadManager(peer, pieceStorage, file.infoHash);
                try {
                    mng.EstablishConnection();
                    cnt++;
                    sec = 2;
                } catch (std::runtime_error& exc) {
                    // std::cout << "connect failed... " << exc.what() << std::endl;;
                    mng.Terminate();
                    if (sec > 8)
                        break;
                    std::this_thread::sleep_for(sec * 1s);
                    sec *= 2;
                    continue;
                }

                auto t1 = std::thread([&mng, &pieceStorage]() {
                    try {
                        mng.ReceiveLoop();
                    } catch (std::runtime_error& exc) {
                        // std::cout << "recv loop failed, terminating... " << exc.what() << std::endl;
                    }
                    mng.Terminate();
                });
                auto t2 = std::thread([&mng, &pieceStorage]() {
                    try {
                        mng.SendLoop();
                    } catch (std::runtime_error& exc) {
                        // std::cout << "send loop failed, terminating... " << exc.what() << std::endl;
                    }
                    mng.Terminate();
                });
                t1.join();
                t2.join();
            }
        });
        std::this_thread::sleep_for(300ms);
    }

    for (auto* ptr : mngs)
        ptr->Terminate();

    for (auto& t : threads)
        t.join();

    return 0;
}