#include "piece_storage.h"
#include <iostream>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>
#include <random>
#include <filesystem>
#include <algorithm>

using namespace std::chrono_literals;

PieceStorage::PieceStorage(const TorrentFile& torrentFile, std::filesystem::path outputPath) 
        : torrentFile_(torrentFile), totalCount_(torrentFile.info.pieces.size()), 
        savedState_(torrentFile.info.pieces.size()), outputPath_(outputPath) {
    std::vector<std::shared_ptr<Piece>> piecesOrd;
    piecesOrd.reserve(torrentFile.info.pieces.size());

    size_t torrentSize = torrentFile.CalculateSize();
    size_t c = 0;
    for (size_t i = 0; i < torrentFile.info.pieces.size(); i++) {
        piecesOrd.push_back(std::make_shared<Piece>(
            i,
            std::min(torrentSize, (i + 1) * torrentFile.info.pieceLength) - i * torrentFile.info.pieceLength,
            torrentFile.info.pieces[i]
        ));
        c++;
    }

    std::default_random_engine rng;
    std::shuffle(piecesOrd.begin(), piecesOrd.end(), rng);

    for (auto piece : piecesOrd)
        piecesQueue_.push(piece);
    
    InitFileStreams();
}

void PieceStorage::InitFileStreams() {
    outputFilesSizesPref_.push_back(0);
    if (std::holds_alternative<TorrentFile::MultiFileStructure>(torrentFile_.info.structure)) {
        auto& structure = std::get<TorrentFile::MultiFileStructure>(torrentFile_.info.structure);
        for (auto& file : structure.files) {
            std::filesystem::path path = outputPath_ / structure.name;
            for (const std::string& s : file.path)
                path /= s;
            std::filesystem::create_directories(path.parent_path());

            outputFiles_.emplace_back(path);
            outputFilesSizesPref_.push_back(outputFilesSizesPref_.back() + file.length);
        }

        outputFilesMtxs_ = std::vector<std::mutex>(outputFiles_.size());
    } else if (std::holds_alternative<TorrentFile::SingleFileStructure>(torrentFile_.info.structure)) {
        auto& structure = std::get<TorrentFile::SingleFileStructure>(torrentFile_.info.structure);
        std::filesystem::path path = outputPath_ / structure.name;

        outputFiles_.emplace_back(path);
        outputFilesSizesPref_.push_back(structure.length);
        outputFilesMtxs_ = std::vector<std::mutex>(outputFiles_.size());
    } else {
        throw std::runtime_error("unknown torrent file structure");
    }
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
    std::lock_guard lock(queueMtx_);
    if (!piecesQueue_.empty()) {
        auto piece = piecesQueue_.front();
        piecesQueue_.pop();
        pendingCount_++;
        
        return piece;
    } else {

        return nullptr;
    }
}

void PieceStorage::SavePieceToFile(std::shared_ptr<Piece> piece) {
    size_t pieceL = piece->GetIndex() * torrentFile_.info.pieceLength;
    size_t pieceR = pieceL + piece->GetData().size();

    // note: [pieceL, pieceR)

    size_t fileIndex = std::upper_bound(
        outputFilesSizesPref_.begin(),
        outputFilesSizesPref_.end(),
        pieceL
    ) - outputFilesSizesPref_.begin() - 1;

    while (
        fileIndex < outputFiles_.size() &&
        outputFilesSizesPref_[fileIndex] < pieceR
    ) {
        std::lock_guard lock(outputFilesMtxs_[fileIndex]);

        size_t fileL = outputFilesSizesPref_[fileIndex];
        size_t fileR = outputFilesSizesPref_[fileIndex + 1];

        outputFiles_[fileIndex].seekp(std::max(fileL, pieceL) - fileL);
        outputFiles_[fileIndex].write(
            &piece->GetData()[std::max(fileL, pieceL) - pieceL],
            std::min(pieceR, fileR) - std::max(fileL, pieceL)
        );

        fileIndex++;
    }
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

bool PieceStorage::PieceDownloaded(std::shared_ptr<Piece> piece) {
    std::unique_lock lock(stateMtx_);
    if (savedState_[piece->GetIndex()]) 
        return false;
    lock.unlock();

    if (!piece->HashMatches()) {
        piece->Reset();
        
        queueMtx_.lock();
        piecesQueue_.push(piece);
        queueMtx_.unlock();

        return false;
    } else {
        SavePieceToFile(piece);
        lock.lock();
        savedState_[piece->GetIndex()] = true;
        lock.unlock();
        
        finishedCount_++;
        pendingCount_--;

        return true;
    }
}