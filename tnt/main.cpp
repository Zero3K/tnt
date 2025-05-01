
#include <fstream>
#include <iostream>
#include "torrent_file/parser.h"
#include "torrent_file/types.h"

int main() {
    std::cout << "insert torrent file path here: ";
    std::string path;
    std::cin >> path;

    TorrentFile::Metainfo file;
    std::ifstream stream(path);
    stream >> file;

    std::cout << "Parsed:" << std::endl;
    std::cout << " - " << file.name << std::endl;
    std::cout << " - " << file.announce << std::endl;
    std::cout << " - " << file.comment << std::endl;
    std::cout << "hash: " << std::hex;
    for (int i = 0; i < 20; i++) {
        std::cout << std::setfill('0') << std::setw(2) 
            << uint32_t(abs(file.infoHash[i])) << " ";
    }
    std::cout << "\n";
    
    return 0;
}