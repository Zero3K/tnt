#pragma once

#include "../peer_connection.h"
#include "../thread_pool.h"
#include <mutex>


class DownloadManager {
public:
    explicit DownloadManager(ThreadPool& threadPool, std::vector<Peer> peers, PieceStorage& storage, std::string hash);

    void Run();

private:
    void EstablishConnections();

    ThreadPool& threadPool_;
    mutable std::mutex mtx_;
    std::vector<PeerConnection> cons_;
    PieceStorage& storage_;
};