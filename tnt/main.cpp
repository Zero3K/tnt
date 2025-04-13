
#include <iostream>
#include "parser/parser.h"

// testing
int main() {
	std::cout << "insert torrent file path here: ";
	std::string path;
	std::cin >> path;

	TorrentFile file = LoadTorrentFile(path);

	std::cout << "Parsed:" << std::endl;
	std::cout << " - " << file.name << std::endl;
	std::cout << " - " << file.announce << std::endl;
	std::cout << " - " << file.comment << std::endl;
	
	return 0;
}