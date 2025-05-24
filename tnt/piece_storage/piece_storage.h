#pragma once

#include "../torrent_file/types.h"
#include "piece.h"
#include <queue>
#include <string>
#include <set>
#include <mutex>
#include <chrono>
#include <map>
#include <filesystem>
#include <fstream>


class PieceStorage {
public:
    explicit PieceStorage(const TorrentFile& tf, std::ofstream& outputFile);

    std::shared_ptr<Piece> AcquirePiece();

    void PieceDownloaded(std::shared_ptr<Piece> piece);

    bool AllPiecesDownloaded() const;

    size_t GetDownloadedCount() const;

    void SavePieceToFile(std::shared_ptr<Piece> piece);
private:
    std::set<std::pair<std::chrono::time_point<std::chrono::steady_clock>, std::shared_ptr<Piece>>> pendingPieces_;
    std::vector<std::chrono::time_point<std::chrono::steady_clock>> acquireTimes_;
    std::vector<std::shared_ptr<Piece>> allPieces_;
    std::atomic<size_t> goodCount_ = 0;
    TorrentFile tf_;
    std::ofstream& outputFile;

    mutable std::mutex mtx_;
};