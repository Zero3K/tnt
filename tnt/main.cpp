#include "piece_storage/piece_storage.h"
#include "torrent_file/parser.h"
#include "torrent_file/types.h"
#include "torrent_tracker.h"
#include "peer_connection.h"
#include "thread_pool.h"
#include "download_manager/download_manager.h"
#include <exception>
#include <fstream>
#include <iostream>
#include <ostream>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

int main(int argc, char **argv) {
    std::cout << "insert torrent file path here: ";
    std::string path;
    std::cin >> path;

    TorrentFile file;
    std::ifstream stream(path);
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

    std::vector<std::thread> threads;
    std::vector<DownloadManager> mngs;
    // very shitty, i'll fix that i promise
    for (auto peer : peers) {
        threads.emplace_back([&]() {
            std::atomic<int> cnt = 2;

            auto mng = DownloadManager(peer, pieceStorage, file.infoHash);
            try {
                mng.EstablishConnection();
                // mng.ReceiveLoop();
                threads.emplace_back([&]() {
                    try {
                        mng.ReceiveLoop();
                    } catch (std::runtime_error& exc) {
                        std::cout << "recv loop failed, terminating... " << exc.what() << std::endl;
                        mng.Terminate();
                        cnt--;
                    }
                });
                threads.emplace_back([&]() {
                    try {
                        mng.SendLoop();
                    } catch (std::runtime_error& exc) {
                        std::cout << "send loop failed, terminating... " << exc.what() << std::endl;
                        mng.Terminate();
                        cnt--;
                    }
                });
                cnt.wait(2);
                cnt.wait(1);
                mng.Terminate();
            } catch (std::runtime_error& exc) {
                std::cout << "connect failed... " << exc.what() << std::endl;
            }
        });
    }
    
    for (auto& t : threads)
        t.join();

    return 0;
}