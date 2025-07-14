#include "piece_storage.h"
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>

using namespace std::chrono_literals;

PieceStorage::PieceStorage(const TorrentFile& tf, std::ofstream& outputFile) 
        : tf_(tf), outputFile_(outputFile), totalCount_(tf.pieceHashes.size()), 
        savedState_(tf.pieceHashes.size()) {
    std::vector<std::shared_ptr<Piece>> piecesOrd;
    piecesOrd.reserve(tf.pieceHashes.size());

    for (size_t i = 0; i < tf.pieceHashes.size(); i++) {
        piecesOrd.push_back(std::make_shared<Piece>(
            i,
            std::min(tf.length, (i + 1) * tf.pieceLength) - i * tf.pieceLength,
            tf.pieceHashes[i]
        ));
    }

    std::default_random_engine rng;
    std::shuffle(piecesOrd.begin(), piecesOrd.end(), rng);

    for (auto piece : piecesOrd)
        piecesQueue_.push(piece);

    ReserveSpace(outputFile_, tf.length);
}

void PieceStorage::ReserveSpace(std::ofstream& file, size_t bytesCount) {
    std::vector<char> zeros(1024, 0);
    for (size_t i = 0; i < bytesCount; i += 1024) {
        file.seekp(i);
        file.write(&zeros[0], std::min(bytesCount, i + 1024) - i);
    }
    file.seekp(0);
}

std::shared_ptr<Piece> PieceStorage::AcquirePiece() {
    if (!piecesQueue_.empty()) {
        queueMtx_.lock();
        auto piece = piecesQueue_.front();
        piecesQueue_.pop();
        pendingCount_++;
        queueMtx_.unlock();
        
        return piece;
    } else {
        return nullptr;
    }
}

void PieceStorage::SavePieceToFile(std::shared_ptr<Piece> piece) {
    std::lock_guard lock(fileMtx_);
    outputFile_.seekp(piece->GetIndex() * tf_.pieceLength);
    auto data = piece->GetData();
    outputFile_.write(&data[0], data.size());
}

size_t PieceStorage::GetTotalCount() const {
    return totalCount_;
}

size_t PieceStorage::GetFinishedCount() const {
    return finishedCount_;
}

size_t PieceStorage::GetPendingCount() const {
    return pendingCount_;
}

size_t PieceStorage::GetQueuedCount() const {
    std::lock_guard lock(queueMtx_);
    return piecesQueue_.size();
}

void PieceStorage::PieceDownloaded(std::shared_ptr<Piece> piece) {
    std::unique_lock lock(stateMtx_);
    if (savedState_[piece->GetIndex()]) 
        return;

    if (!piece->HashMatches()) {
        piece->Reset();
        
        queueMtx_.lock();
        piecesQueue_.push(piece);
        queueMtx_.unlock();
    } else {
        SavePieceToFile(piece);
        savedState_[piece->GetIndex()] = true;
        lock.unlock();
        
        finishedCount_++;
        pendingCount_--;
    }
}