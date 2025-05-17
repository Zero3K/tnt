#pragma once

#include "../peer_connection/peer_connection.h"
#include <mutex>
#include <set>


class DownloadManager {
public:
    explicit DownloadManager(Peer peer, PieceStorage& storage, std::string hash);

    void ReceiveLoop();
    void SendLoop();
    void EstablishConnection();
    void Terminate();
private:
    void RequestBlocksForPiece(std::shared_ptr<Piece> piece);

    mutable std::mutex mtx_;
    PeerConnection con_;
    PieceStorage* storage_;

    std::set<std::shared_ptr<Piece>> requestedPieces_;

    std::atomic<bool> choked_ = true;
    std::atomic<bool> terminating_ = false;
    std::atomic<bool> sendRunning_ = false;
    std::atomic<bool> recvRunning_ = false;
};