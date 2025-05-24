#include "piece_storage/piece_storage.h"
#include "torrent_file/parser.h"
#include "torrent_file/types.h"
#include "torrent_tracker.h"
#include "peer_connection/peer_connection.h"
#include "download_manager/download_manager.h"
#include "codes.h"
#include <cxxopts.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <ostream>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;
namespace fs = std::filesystem;

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);

    std::cout.precision(1);
    std::cout.setf(std::ios::fixed);

    // =========================================================

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

    // =========================================================

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

    // =========================================================

    std::vector<std::thread> threads;

    std::chrono::time_point tpStart = std::chrono::steady_clock::now();

    std::atomic<int> connCnt = 0;
    auto downloadProgressBarJob = [&]() {
        std::cout << "\n\n\n";
        while (true) {
            int downloadedCnt = pieceStorage.GetDownloadedCount();
            std::cout << MOVE_UP << MOVE_UP << RESET << "\r";

            int progress = downloadedCnt * 50 / file.pieceHashes.size();
            std::cout << CLEAR_LINE;
            std::cout << BOLD << LIGHT_GRAY << "peers connected: " << YELLOW << connCnt << "\n";
            std::cout << CLEAR_LINE;
            std::cout << GREEN << "downloading... [";
            
            for (int i = 0; i < progress - 1; i++) 
                std::cout << "=";
            if (progress > 0 && progress != 50)
                std::cout << ">";

            for (int i = 0; i < 50 - progress; i++) 
                std::cout << " ";
            std::cout << "] " << downloadedCnt << "/" << file.pieceHashes.size() << " ";

            std::chrono::time_point tpCur = std::chrono::steady_clock::now();

            // TODO: rework
            float avgSpeed = 1000.0 * downloadedCnt * file.pieceLength /
                std::chrono::duration_cast<std::chrono::milliseconds>(tpCur - tpStart).count() / 1024 / 1024;
            std::cout << avgSpeed << " MB/s\n";

            if (progress == 50)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    };
    
    threads.emplace_back(downloadProgressBarJob);

    auto dmReceiveTask = [](DownloadManager& mng) {
        try {
            mng.ReceiveLoop();
        } catch (std::runtime_error& exc) {
            // ...
        }
    };

    auto dmSendTask = [](DownloadManager& mng) {
        try {
            mng.SendLoop();
        } catch (std::runtime_error& exc) {
            // ...
        }
    };

    auto downloadTask = [&](Peer peer) {
        int attempts = 0;
        while (!pieceStorage.AllPiecesDownloaded()) {
            auto mng = DownloadManager(peer, pieceStorage, file.infoHash);
            try {
                mng.EstablishConnection();
                attempts = 0;
            } catch (std::runtime_error& exc) {
                attempts++;
                mng.Terminate();

                if (attempts > 3)
                    break;
                continue;
            }

            connCnt++;

            auto t1 = std::thread(dmReceiveTask, std::ref(mng));
            auto t2 = std::thread(dmSendTask, std::ref(mng));

            t1.join();
            t2.join();

            connCnt--;
        }
    };

    // very shitty, i'll fix that, i promise
    for (auto peer : peers) {
        threads.emplace_back(downloadTask, peer);
        std::this_thread::sleep_for(100ms);
    }

    for (auto& t : threads)
        t.join();

    return 0;
}