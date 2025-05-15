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

    ThreadPool pool(50);
    DownloadManager manager(pool, peers, pieceStorage, file.infoHash);
    manager.Run();
    pool.Terminate(true);

    return 0;
}