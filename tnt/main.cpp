#include "piece_storage/piece_storage.h"
#include "torrent_file/parser.h"
#include "torrent_file/types.h"
#include "torrent_tracker.h"
#include "peer_connection/peer_connection.h"
#include "download_manager/download_manager.h"
#include <cxxopts.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <ostream>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

int main(int argc, char **argv) {
    cxxopts::Options options("tnt", "Toy torrent client");
    options.add_options()
        ("file", "Metainfo (.torrent) file.", cxxopts::value<std::string>());

    options.parse_positional({"file"});
    auto result = options.parse(argc, argv);

    std::string torrentFilePath = result["file"].as<std::string>();
    
    TorrentFile file;
    std::ifstream stream(torrentFilePath);
    stream >> file;

    std::cout << "brief:" << std::endl;
    std::cout << " - " << file.name << std::endl;
    std::cout << " - " << file.announce << std::endl;
    std::cout << " - " << file.comment << std::endl << std::endl;
    std::cout << " - piece size: " << file.pieceLength << std::endl;
    std::cout << " - pieces: " << file.pieceHashes.size() << std::endl << std::endl;

    std::cout << "getting peers..." << std::flush; 
    TorrentTracker tracker(file.announce);
    tracker.UpdatePeers(file, "TEST0APP1DONT2WORRY3", 12345);
    auto& peers = tracker.GetPeers();
    std::cout << " found " << peers.size() << " peers" << std::endl;

    PieceStorage pieceStorage(file);

    std::mutex mtx;
    std::vector<std::thread> threads;
    std::vector<DownloadManager*> mngs;
    
    threads.emplace_back([&]() {
        std::cout << '\n';
        while (!pieceStorage.AllPiecesGood()) {
            std::this_thread::sleep_for(100ms);
            std::cout << "\rpieces retrieved: " << pieceStorage.GetProcessedCount() << std::flush;
        }
        std::cout << "\n";
    });

    // very shitty, i'll fix that, i promise
    for (auto peer : peers) {
        if (pieceStorage.AllPiecesGood())
            break;
        threads.emplace_back([&]() {
            while (!pieceStorage.AllPiecesGood()) {
                auto mng = DownloadManager(peer, pieceStorage, file.infoHash);
                try {
                    mng.EstablishConnection();
                } catch (std::runtime_error& exc) {
                    // std::cout << "connect failed... " << exc.what() << std::endl;;
                    mng.Terminate();
                    break;
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
                // std::cout << "reconnect\n";
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