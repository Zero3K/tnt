#include "piece.h"
#include <iostream>
#include <algorithm>
#include <openssl/sha.h>


static constexpr size_t BLOCK_SIZE = 1 << 14;

Piece::Piece(size_t index, size_t length, std::string hash) : 
        index_(index), length_(length), hash_(hash) {
    blocks_ = std::vector<Block>((length + BLOCK_SIZE - 1) / BLOCK_SIZE);
    for (int i = 0; i < blocks_.size(); i++) {
        blocks_[i].piece = index_;
        blocks_[i].offset = i * BLOCK_SIZE;
        blocks_[i].length = std::min<size_t>(length_, (i + 1) * BLOCK_SIZE) - i * BLOCK_SIZE;
        blocks_[i].status = Block::Status::Missing;
    }
}

/*
* Совпадает ли хеш скачанных данных с ожидаемым
*/
bool Piece::HashMatches() const {
    return GetDataHash() == GetHash();
}

/*
    * Дать указатель на отсутствующий (еще не скачанный и не запрошенный) блок
    */
Block* Piece::FirstMissingBlock() {
    auto it = std::find_if(blocks_.begin(), blocks_.end(), [&](const Block& block) {
        return block.status != Block::Status::Retrieved; 
    });

    if (it == blocks_.end())
        throw std::runtime_error("there is no other block");
    return &*it;
}

/*
    * Получить порядковый номер части файла
    */
size_t Piece::GetIndex() const {
    return index_;
}

/*
    * Сохранить скачанные данные для какого-то блока
    */
void Piece::SaveBlock(size_t blockOffset, std::string data) {
    blocks_[blockOffset / BLOCK_SIZE].data = data;
    blocks_[blockOffset / BLOCK_SIZE].status = Block::Status::Retrieved;
}

/*
    * Скачали ли уже все блоки
    */
bool Piece::AllBlocksRetrieved() const {
    return std::count_if(blocks_.begin(), blocks_.end(), [&](const Block& block) {
        return block.status == Block::Status::Retrieved; 
    }) == blocks_.size();
}

/*
    * Получить скачанные данные для части файла
    */
std::string Piece::GetData() const {
    std::string result;
    for (const Block& block : blocks_) {
        auto c = block.data;
        result += c;
    }
    return result;
}

/*
    * Посчитать хеш по скачанным данным
    */
std::string Piece::GetDataHash() const {
    std::string result(20, 0);
    std::string data = GetData();
    SHA1(
        reinterpret_cast<unsigned char*>(&data[0]), 
        data.size(), 
        reinterpret_cast<unsigned char*>(&(result[0]))
    );
    return result;
}

/*
    * Получить хеш для части из .torrent файла
    */
const std::string& Piece::GetHash() const {
    return hash_;
}

/*
* Удалить все скачанные данные и отметить все блоки как Missing
*/
void Piece::Reset() {
    for (Block& block : blocks_) {
        block.data = "";
        block.status = Block::Status::Missing;
    }
}
