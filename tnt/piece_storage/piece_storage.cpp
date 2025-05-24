#include "piece_storage.h"
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>

using namespace std::chrono_literals;
constexpr std::chrono::duration maxPieceWaitTime = 4000ms;

PieceStorage::PieceStorage(const TorrentFile& tf, std::ofstream& outputFile) 
        : tf_(tf), allPieces_(tf.pieceHashes.size()), acquireTimes_(tf.pieceHashes.size()), outputFile(outputFile) {
    std::vector<char> zeros(1024, 0);
    for (size_t i = 0; i < tf.length; i += 1024) {
        outputFile.seekp(i);
        outputFile.write(&zeros[0], std::min(tf.length, i + 1024) - i);
    }

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

bool PieceStorage::AllPiecesDownloaded() const {
    return pendingPieces_.empty();
}

void PieceStorage::PieceDownloaded(std::shared_ptr<Piece> piece) {
    std::lock_guard lock(mtx_);
    if (pendingPieces_.find({ acquireTimes_[piece->GetIndex()], piece }) != pendingPieces_.end()) {
        pendingPieces_.erase({ acquireTimes_[piece->GetIndex()], piece });
        SavePieceToFile(piece);
        // std::cout << "piece " << goodCount_ << "\n";
        goodCount_++;
    }
}

size_t PieceStorage::GetDownloadedCount() const {
    return goodCount_;
}

void PieceStorage::SavePieceToFile(std::shared_ptr<Piece> piece) {
    outputFile.seekp(piece->GetIndex() * tf_.pieceLength);
    auto data = piece->GetData();
    outputFile.write(&data[0], data.size());
}