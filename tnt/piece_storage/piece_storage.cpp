#include "piece_storage.h"
#include <iostream>
#include <memory>


PieceStorage::PieceStorage(const TorrentFile& tf) {
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
    auto ptr = remainPieces_.front();
    remainPieces_.pop();
    return ptr;
}

void PieceStorage::PieceProcessed(std::shared_ptr<Piece> piece) {
    std::cout << "piece " << piece->GetIndex() << " processed" << std::endl;
}

bool PieceStorage::QueueIsEmpty() const {
    return remainPieces_.empty();
}

// void PieceStorage::SavePieceToDisk(std::shared_ptr<Piece> piece) {
    
// }
