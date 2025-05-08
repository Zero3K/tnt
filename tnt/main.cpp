#include "torrent_file/parser.h"
#include "torrent_file/types.h"
#include "torrent_tracker/torrent_tracker.h"
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
    std::cout << " - " << file.comment << std::endl;

    return 0;
}