#pragma once

#include <vector>
#include <optional>
#include <variant>


struct TorrentFile {
    using AnnounceListType = std::vector<std::vector<std::string>>;

    struct SingleFileStructure {
        std::string name;
        size_t length;
    };

    // will be finished once multifile support added
    struct MultiFileStructure {};

    struct Info {
        size_t pieceLength;
        std::vector<std::string> pieces;
        std::variant<SingleFileStructure, MultiFileStructure> structure;
    } info;
    
    std::string announce;
    std::optional<AnnounceListType> announceList;

    std::optional<std::string> comment;
    std::optional<std::string> author;

    std::string infoHash;
};
