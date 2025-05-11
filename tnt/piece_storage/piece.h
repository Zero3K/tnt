#pragma once

#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <chrono>


struct Block {
    uint32_t piece;
    uint32_t offset;
    uint32_t length;
    std::string data;
};

class Piece {
public:
    Piece(size_t index, size_t length, std::string hash);

    // Returns reference to the first still unretrieved block.
    const Block& GetFirstMissingBlock();

    // Saves bytes passed in `data` with offset equal to `blockOffset`.
    void SaveBlock(size_t blockOffset, std::string data);
    
    // Validates that piece was fully recieved and the data was not corrupted.
    // Returns true if all data is ready and correct.
    bool Validate();

    // Returns piece index.
    size_t GetIndex() const;
private:

    bool AllBlocksRetrieved() const;

    std::string GetData() const;

    std::string GetRetrievedDataHash() const;

    void Reset();

    const size_t index_, length_;
    const std::string hash_;
    std::vector<Block> blocks_;
    size_t retrievedCount_;
};
