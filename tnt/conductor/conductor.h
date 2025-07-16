#pragma once

#include "../peer_downloader/peer_downloader.h"
#include "../piece_storage/piece_storage.h"

class Conductor {
public:
    explicit Conductor(const std::vector<Peer>& peers, const TorrentFile& file, PieceStorage& storage);
    ~Conductor();

    /*
     * Download all the pieces provided by the piece storage.
     */
    void Download();

    /*
     * Get current connected peers count.
     */
    size_t GetConnectedCount() const;

    /*
     * Check if conductor is in endgame mode.
     */
    bool isEndgame();
private:
    /*
     * Connect specified peer to the conductor.
     */
    void ConnectPeer(Peer peer);

    /*
     * Request specified piece from some peer.
     */
    void QueuePiece(std::shared_ptr<Piece> piece);

    /*
     * Request specified piece from all peers.
     */
    void BroadcastPiece(std::shared_ptr<Piece> piece);

    const std::vector<Peer>& peers;
    const TorrentFile& file_;
    PieceStorage& storage_;

    std::atomic<bool> endgame_ = false;
    std::atomic<size_t> connectedPeersCount_ = 0;
    std::vector<std::unique_ptr<PeerDownloader>> downloaders_;
    std::vector<std::thread> threads_;

    mutable std::mutex mtx_;
};