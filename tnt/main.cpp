
#include <fstream>
#include <iostream>
#include "tf_utils/parser.h"

int main() {
    using namespace TFUtils;

    std::cout << "insert torrent file path here: ";
    std::string path;
    std::cin >> path;

    TorrentFile file;
    std::ifstream stream(path);
    stream >> file;

    std::cout << "Parsed:" << std::endl;
    std::cout << " - " << file.name << std::endl;
    std::cout << " - " << file.announce << std::endl;
    std::cout << " - " << file.comment << std::endl;
    
    return 0;
}