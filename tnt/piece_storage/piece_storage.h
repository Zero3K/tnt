#pragma once

#include "../torrent_file/types.h"
#include "piece.h"
#include <queue>
#include <string>
#include <unordered_set>
#include <mutex>


class PieceStorage {
public:
    explicit PieceStorage(const TorrentFile& tf);

    std::shared_ptr<Piece> GetNextPieceToDownload();

    std::shared_ptr<Piece> GetPiece(size_t idx);

    void PieceProcessed(std::shared_ptr<Piece> piece);

    void PieceFailed(std::shared_ptr<Piece> piece);

    bool QueueIsEmpty() const;

    bool AllPiecesGood() const;
    
    
private:
    std::queue<std::shared_ptr<Piece>> remainPieces_;
    std::vector<std::shared_ptr<Piece>> allPieces_;
    mutable std::mutex queueMtx_;
    std::vector<bool> isGood_;
    size_t goodCount_ = 0;
};