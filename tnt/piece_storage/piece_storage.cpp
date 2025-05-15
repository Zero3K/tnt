#include "piece_storage.h"
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>


PieceStorage::PieceStorage(const TorrentFile& tf) : allPieces_(tf.pieceHashes.size()), isGood_(tf.pieceHashes.size()) {
    std::lock_guard lock(queueMtx_);

    for (size_t i = 0; i < tf.pieceHashes.size(); i++) {
        auto ptr = std::make_shared<Piece>(
            i,
            std::min(tf.length, (i + 1) * tf.pieceLength) - i * tf.pieceLength,
            tf.pieceHashes[i]
        );

        remainPieces_.push(ptr);
        allPieces_[i] = ptr;
    }
}

std::shared_ptr<Piece> PieceStorage::GetNextPieceToDownload() {
    std::lock_guard lock(queueMtx_);

    std::shared_ptr<Piece> ptr = nullptr;
    if (!remainPieces_.empty()) {
        ptr = remainPieces_.front();
        remainPieces_.pop();
    }

    return ptr;
}

std::shared_ptr<Piece> PieceStorage::GetPiece(size_t idx) {
    return allPieces_[idx];
}

bool PieceStorage::AllPiecesGood() const {
    return goodCount_ == isGood_.size();
}

void PieceStorage::PieceProcessed(std::shared_ptr<Piece> piece) {
    std::lock_guard lock(queueMtx_);
    if (!isGood_[piece->GetIndex()]) {
        isGood_[piece->GetIndex()] = true;
        goodCount_++;

        std::cout << "piece " << piece->GetIndex() << " processed by thread " << std::this_thread::get_id() << std::endl;
    }
}

void PieceStorage::PieceFailed(std::shared_ptr<Piece> piece) {
    std::lock_guard lock(queueMtx_);
    remainPieces_.push(piece);
    // std::cout << "piece " << piece->GetIndex() << " failed, moving to queue" << std::endl;
}

bool PieceStorage::QueueIsEmpty() const {
    std::lock_guard lock(queueMtx_);
    return remainPieces_.empty();
}