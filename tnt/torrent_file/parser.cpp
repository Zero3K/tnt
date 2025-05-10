#include "../bencode/decoding.h"
#include "../bencode/encoding.h"
#include <iostream>
#include <sstream>
#include <memory>
#include <openssl/sha.h>
#include "types.h"


TorrentFile ParseTorrentFile(std::istream& stream) {
    TorrentFile result;
    
    auto data = std::make_shared<Bencode::Entity>();
    stream >> data;

    auto dataMap = std::static_pointer_cast<Bencode::Dict>(data)->value;
    result.announce = std::static_pointer_cast<Bencode::String>(dataMap["announce"])->value;
    result.comment = std::static_pointer_cast<Bencode::String>(dataMap["comment"])->value;

    auto infoMap = std::static_pointer_cast<Bencode::Dict>(dataMap["info"])->value;
    result.pieceLength = std::static_pointer_cast<Bencode::Integer>(infoMap["piece length"])->value;
    result.length = std::static_pointer_cast<Bencode::Integer>(infoMap["length"])->value;
    result.name = std::static_pointer_cast<Bencode::String>(infoMap["name"])->value;

    std::string pieces = std::static_pointer_cast<Bencode::String>(infoMap["pieces"])->value;
    for (size_t i = 0; i + 20 <= pieces.size(); i += 20)
        result.pieceHashes.push_back(pieces.substr(i, 20));

    std::stringstream encoded;
    Bencode::WriteEntity(encoded, dataMap["info"]);

    result.infoHash.resize(20);
    SHA1(
        reinterpret_cast<unsigned char*>(&encoded.str()[0]), 
        encoded.str().size(), 
        reinterpret_cast<unsigned char*>(&(result.infoHash[0]))
    );

    return result;
}

std::istream& operator>>(std::istream& stream, TorrentFile& file) {
    file = ParseTorrentFile(stream);
    return stream;
}
