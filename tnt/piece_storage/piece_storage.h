#pragma once

#include "../torrent_file/types.h"
#include "piece.h"
#include <queue>
#include <string>
#include <set>
#include <mutex>
#include <chrono>
#include <map>


class PieceStorage {
public:
    explicit PieceStorage(const TorrentFile& tf);

    std::shared_ptr<Piece> AcquirePiece();

    void PieceProcessed(std::shared_ptr<Piece> piece);

    bool AllPiecesGood() const;

    int GetProcessedCount() const;
private:
    std::set<std::pair<std::chrono::time_point<std::chrono::steady_clock>, std::shared_ptr<Piece>>> pendingPieces_;
    std::vector<std::chrono::time_point<std::chrono::steady_clock>> acquireTimes_;
    std::vector<std::shared_ptr<Piece>> allPieces_;
    mutable std::mutex mtx_;
    std::atomic<int> goodCount_ = 0;
};