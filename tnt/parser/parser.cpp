#include <string>
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <map>
#include <memory>
#include <iostream>

#include "parser.h"


std::shared_ptr<TfEntity> ReadEntity(std::ifstream& stream);


std::string TfInteger::Encode() const {
    return std::string("i") + std::to_string(value) + "e";
}

std::shared_ptr<TfInteger> TfInteger::Read(std::ifstream& stream) {
    if (stream.peek() != 'i')
        throw;

    stream.get();  // pop 'i'
    int64_t result = 0;
    while (stream.peek() != 'e')
        result = result * 10 + (stream.get() - '0');
    stream.get();  // pop 'e'

    auto ptr = std::make_shared<TfInteger>();
    ptr->value = result;

    return ptr;
}


std::string TfString::Encode() const {
    return std::to_string(value.size()) + ":" + value;
}

std::shared_ptr<TfString> TfString::Read(std::ifstream& stream) {
    if (!isdigit(stream.peek()))
        throw;
    
    int64_t size = 0;
    while (stream.peek() != ':')
        size = size * 10 + (stream.get() - '0');
    stream.get();  // pop ':'

    auto ptr = std::make_shared<TfString>();
    ptr->value.resize(size);
    stream.read(&(ptr->value[0]), size);

    return ptr;
}


std::string TfList::Encode() const {
    std::string result = "l";
    for (auto entity : contents)
        result += entity->Encode();
    result += "e";
    return result;
}

std::shared_ptr<TfList> TfList::Read(std::ifstream& stream) {
    if (stream.peek() != 'l')
        throw;
    stream.get();  // pop l
    
    auto ptr = std::make_shared<TfList>();
    while (stream.peek() != 'e') {
        auto entity = ReadEntity(stream);
        ptr->contents.push_back(entity);
    }

    stream.get();  // pop e

    return ptr;
}


std::string TfDict::Encode() const {
    std::string result = "d";
    for (auto [key, val] : contents) {
        TfString str;
        str.value = key;

        result += str.Encode();
        result += val->Encode();
    }

    result += "e";
    return result;
}

std::shared_ptr<TfDict> TfDict::Read(std::ifstream& stream) {
    if (stream.peek() != 'd')
        throw;
    stream.get();  // pop d
    
    auto ptr = std::make_shared<TfDict>();
    while (stream.peek() != 'e') {
        auto key = ReadEntity(stream);
        if (std::dynamic_pointer_cast<TfString>(key) == nullptr)
            throw;
        
        auto value = ReadEntity(stream);
        ptr->contents[std::static_pointer_cast<TfString>(key)->value] = value;
    }

    stream.get();  // pop e

    return ptr;
}


std::shared_ptr<TfEntity> ReadEntity(std::ifstream& stream) {
    if (isdigit(stream.peek())) {
        return TfString::Read(stream);
    } else if (stream.peek() == 'i') {
        return TfInteger::Read(stream);
    } else if (stream.peek() == 'l') {
        return TfList::Read(stream);
    } else if (stream.peek() == 'd') {
        return TfDict::Read(stream);
    } else {
        throw;
    }
}

void PrintEntity(std::shared_ptr<TfEntity> ptr) {
    if (std::dynamic_pointer_cast<TfString>(ptr))
        std::cout << "\"" << std::static_pointer_cast<TfString>(ptr)->value << "\"";
    else if (std::dynamic_pointer_cast<TfInteger>(ptr))
        std::cout << std::static_pointer_cast<TfInteger>(ptr)->value;
    else if (std::dynamic_pointer_cast<TfDict>(ptr)) {
        auto data = std::static_pointer_cast<TfDict>(ptr)->contents;
        std::cout << "{";
        for (auto [f, s] : data) {
            std::cout << "\"" << f << "\"" << ": ";
            PrintEntity(s);
            std::cout << ", ";
        }
        std::cout << "}";
    } else if (std::dynamic_pointer_cast<TfList>(ptr)) {
        auto data = std::static_pointer_cast<TfList>(ptr)->contents;
        std::cout << "[";
        for (auto e : data) {
            PrintEntity(e);
            std::cout << ", ";
        }
        std::cout << "]";
    }
}

TorrentFile LoadTorrentFile(const std::string& filename) {
    TorrentFile result;
    std::ifstream stream(filename);
    
    auto data = std::static_pointer_cast<TfDict>(ReadEntity(stream));

    result.announce = std::static_pointer_cast<TfString>(data->contents["announce"])->value;
    result.comment = std::static_pointer_cast<TfString>(data->contents["comment"])->value;

    auto infoDict = std::static_pointer_cast<TfDict>(data->contents["info"]);
    result.pieceLength = std::static_pointer_cast<TfInteger>(infoDict->contents["piece length"])->value;
    result.length = std::static_pointer_cast<TfInteger>(infoDict->contents["length"])->value;
    result.name = std::static_pointer_cast<TfString>(infoDict->contents["name"])->value;
    std::string pieces = std::static_pointer_cast<TfString>(infoDict->contents["pieces"])->value;
    for (size_t i = 0; i + 20 <= pieces.size(); i += 20)
        result.pieceHashes.push_back(pieces.substr(i, 20));

    std::string infoString = infoDict->Encode();
    result.infoHash.resize(20);
    SHA1((unsigned const char*) infoString.c_str(), infoString.size(), (unsigned char*) &(result.infoHash[0]));
    
    return result;
}
