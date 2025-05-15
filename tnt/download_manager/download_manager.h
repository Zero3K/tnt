#pragma once

#include "../peer_connection.h"
#include "../thread_pool.h"
#include <mutex>
#include <set>


class DownloadManager {
public:
    explicit DownloadManager(ThreadPool& threadPool, std::vector<Peer> peers, PieceStorage& storage, std::string hash);

    void Run();

private:
    void EstablishConnections();
    void StartDownloading();
    void RequestBlocksForPiece(PeerConnection& con, std::shared_ptr<Piece> piece);
    void DownloadLoop();

    ThreadPool& threadPool_;
    mutable std::mutex mtx_;
    std::vector<PeerConnection> cons_;
    std::set<PeerConnection*> availableCons_;
    PieceStorage& storage_;
};