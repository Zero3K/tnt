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

    /* 
     * Acquires a piece for downloading.
     */
    std::shared_ptr<Piece> AcquirePiece();

    /* 
     * Called when a piece is downloaded.
     */
    bool PieceDownloaded(std::shared_ptr<Piece> piece);

    /* 
     * Returns the number of pieces in torrent file.
     */
    size_t GetTotalCount() const;

    /* 
     * Returns count of pieces that were already validated and saved to file.
     */
    size_t GetFinishedCount() const;

    /* 
     * Returns count of pieces that were acquired but are not saved yet.
     */
    size_t GetPendingCount() const;

    /* 
     * Returns count of pieces that are currently queued for saving.
     */
    size_t GetQueuedCount() const;
private:
    /* 
     * Saves piece to file.
     */
    void SavePieceToFile(std::shared_ptr<Piece> piece);

    /*
     * Reserves space for a file on disk (by filling it with zeros).
     */
    void ReserveSpace(std::ofstream& file, size_t bytesCount);

    std::queue<std::shared_ptr<Piece>> piecesQueue_;
    std::vector<bool> savedState_;

    size_t totalCount_ = 0;
    std::atomic<size_t> finishedCount_ = 0;
    std::atomic<size_t> pendingCount_ = 0;

    TorrentFile tf_;
    std::ofstream& outputFile_;

    mutable std::mutex queueMtx_, fileMtx_, stateMtx_;
};