#include "piece_storage/piece_storage.h"
#include "torrent_file/parser.h"
#include "torrent_file/types.h"
#include "torrent_tracker.h"
#include "peer_connection.h"
#include <exception>
#include <fstream>
#include <iostream>
#include <ostream>


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
    std::cout << " - pieces: " << file.pieceHashes.size() << std::endl << std::endl;

    std::cout << "getting peers..." << std::flush; 
    TorrentTracker tracker(file.announce);
    tracker.UpdatePeers(file, "TEST0APP1DONT2WORRY3", 12345);
    auto& peers = tracker.GetPeers();
    std::cout << " found " << peers.size() << " peers" << std::endl;

    PieceStorage pieceStorage(file);
    for (const Peer& peer : peers) {
        std::cout << "trying to reach " << peer.ip << ":" << peer.port << "... " << std::flush;
        try {
            PeerConnection conn(peer, file, "TEST0APP1DONT2WORRY3", pieceStorage);
            conn.Run();
            std::cout << "success!" << std::endl;
            conn.Terminate();
            break;
        } catch (std::runtime_error error) {
            std::cout << "runtime: " << error.what() << std::endl;
        } catch (std::exception error) {
            std::cout << "exc: " << error.what() << std::endl;
        }
    }

    return 0;
}