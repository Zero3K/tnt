#include "piece_storage.h"
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>

using namespace std::chrono_literals;
constexpr std::chrono::duration maxPieceWaitTime = 500ms;

PieceStorage::PieceStorage(const TorrentFile& tf) : allPieces_(tf.pieceHashes.size()), acquireTimes_(tf.pieceHashes.size()) {
    std::lock_guard lock(mtx_);

    for (size_t i = 0; i < tf.pieceHashes.size(); i++) {
        auto ptr = std::make_shared<Piece>(
            i,
            std::min(tf.length, (i + 1) * tf.pieceLength) - i * tf.pieceLength,
            tf.pieceHashes[i]
        );

        acquireTimes_[i] = std::chrono::steady_clock::now() - maxPieceWaitTime - 1s;
        pendingPieces_.insert({ acquireTimes_[i], ptr });
        allPieces_[i] = ptr;
    }
}

std::shared_ptr<Piece> PieceStorage::AcquirePiece() {
    std::lock_guard lock(mtx_);

    auto it = pendingPieces_.begin();
    if (it != pendingPieces_.end() && (std::chrono::steady_clock::now() - it->first) > maxPieceWaitTime) {
        auto [time, piece] = *it;
        acquireTimes_[piece->GetIndex()] = std::chrono::steady_clock::now();
        pendingPieces_.erase(it);
        pendingPieces_.insert({ acquireTimes_[piece->GetIndex()], piece });
        return piece;
    } else {
        return nullptr;
    }
}

std::shared_ptr<Piece> PieceStorage::GetPiece(size_t idx) {
    return allPieces_[idx];
}

bool PieceStorage::AllPiecesGood() const {
    return pendingPieces_.empty();
}

void PieceStorage::PieceProcessed(std::shared_ptr<Piece> piece) {
    std::lock_guard lock(mtx_);
    if (pendingPieces_.find({ acquireTimes_[piece->GetIndex()], piece }) != pendingPieces_.end()) {
        pendingPieces_.erase({ acquireTimes_[piece->GetIndex()], piece });
        std::cout << "piece " << piece->GetIndex() << " processed by thread " << std::this_thread::get_id() << std::endl;
    }
}