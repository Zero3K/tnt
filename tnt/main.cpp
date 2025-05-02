
#include <fstream>
#include <iostream>
#include <ostream>
#include "torrent_file/parser.h"
#include "torrent_file/types.h"
#include "torrent_tracker/torrent_tracker.h"


int main() {
    using namespace TorrentTracker;

    std::cout << "insert torrent file path here: ";
    std::string path;
    std::cin >> path;

    TorrentFile::Metainfo file;
    std::ifstream stream(path);
    stream >> file;

    std::cout << "brief:" << std::endl;
    std::cout << " - " << file.name << std::endl;
    std::cout << " - " << file.announce << std::endl;
    std::cout << " - " << file.comment << std::endl;

    std::ios_base::fmtflags f(std::cout.flags());
    std::cout << "hash: " << std::hex << std::setfill('0') << std::setw(2);
    for (int i = 0; i < 20; i++) {
        std::cout << uint32_t(abs(file.infoHash[i])) << " ";
    }
    std::cout.flags(f);

    std::cout << std::endl << std::endl;

    std::cout << "trying to get peers..." << std::endl;
    Tracker tracker(file.announce);
    tracker.UpdatePeers(file, "TEST0APP1DONT2WORRY3", 12345);

    std::cout << "found " << tracker.GetPeers().size() << " peers!" << std::endl;

    for (const Peer& p : tracker.GetPeers()) {
        std::cout << p.ip << ":" << p.port << std::endl;
    }

    return 0;
}