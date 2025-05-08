#include "torrent_file/parser.h"
#include "torrent_file/types.h"
#include "torrent_tracker/torrent_tracker.h"
#include "peer_connection/peer_connection.h"
#include <exception>
#include <fstream>
#include <iostream>
#include <ostream>


int main(int argc, char **argv) {
    std::cout << "insert torrent file path here: ";
    std::string path;
    std::cin >> path;

    TorrentFile::Metainfo file;
    std::ifstream stream(path);
    stream >> file;

    std::cout << "brief:" << std::endl;
    std::cout << " - " << file.name << std::endl;
    std::cout << " - " << file.announce << std::endl;
    std::cout << " - " << file.comment << std::endl << std::endl;

    std::cout << "getting peers..." << std::flush; 
    TorrentTracker::Tracker tracker(file.announce);
    tracker.UpdatePeers(file, "TEST0APP1DONT2WORRY3", 12345);
    auto& peers = tracker.GetPeers();
    std::cout << " found " << peers.size() << " peers" << std::endl;

    for (const TorrentTracker::Peer& peer : peers) {
        std::cout << "trying to reach " << peer.ip << ":" << peer.port << "... " << std::flush;
        try {
            PeerConnect conn(peer, file, "TEST0APP1DONT2WORRY3");
            conn.Run();
            std::cout << "success!" << std::endl;
            conn.Terminate();
        } catch (std::exception error) {
            std::cout << "fail: " << error.what() << std::endl;
        }
    }

    return 0;
}