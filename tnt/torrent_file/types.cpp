#include "../bencode/decoding.h"
#include "../bencode/encoding.h"
#include "types.h"
#include <iostream>
#include <sstream>
#include <memory>
#include <numeric>

size_t TorrentFile::CalculateSize() const {
    if (std::holds_alternative<SingleFileStructure>(info.structure)) {
        return std::get<SingleFileStructure>(info.structure).length;
    } else {
        size_t result = 0;
        for (auto& file : std::get<MultiFileStructure>(info.structure).files)
            result += file.length;
        return result;
    }
}