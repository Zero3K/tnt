#pragma once

#include <vector>


namespace TorrentFile {
	struct Metainfo {
        std::string announce;
        std::string comment;
        std::vector<std::string> pieceHashes;
        size_t pieceLength;
        size_t length;
        std::string name;
        std::string infoHash;
    };
}