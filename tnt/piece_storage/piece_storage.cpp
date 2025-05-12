#include "piece_storage.h"
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>


PieceStorage::PieceStorage(const TorrentFile& tf) {
    std::lock_guard lock(queueMtx_);

    for (size_t i = 0; i < tf.pieceHashes.size(); i++) {
        remainPieces_.push(
            std::make_shared<Piece>(
                i,
                std::min(tf.length, (i + 1) * tf.pieceLength) - i * tf.pieceLength,
                tf.pieceHashes[i]
            )
        );
    }
}

std::shared_ptr<Piece> PieceStorage::GetNextPieceToDownload() {
    std::lock_guard lock(queueMtx_);

    auto ptr = remainPieces_.front();
    remainPieces_.pop();
    return ptr;
}

void PieceStorage::PieceProcessed(std::shared_ptr<Piece> piece) {
    std::lock_guard lock(queueMtx_);
    std::cout << "piece " << piece->GetIndex() << " processed by thread " << std::this_thread::get_id() << std::endl;
}

bool PieceStorage::QueueIsEmpty() const {
    std::lock_guard lock(queueMtx_);
    return remainPieces_.empty();
}