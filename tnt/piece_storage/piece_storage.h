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

    void PieceProcessed(std::shared_ptr<Piece> piece);

    bool QueueIsEmpty() const;

protected:
    std::queue<std::shared_ptr<Piece>> remainPieces_;
};
