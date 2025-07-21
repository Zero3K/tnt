#pragma once

#include "../peer_connection/peer_connection.h"
#include "../piece_storage/piece.h"
#include <mutex>
#include <set>
#include <future>


class PeerDownloader {
public:
    explicit PeerDownloader(Peer peer, TorrentFile file, 
        std::function<void(std::shared_ptr<Piece>)> callback);
    
    /* 
     * Establishes connection with peer and starts the downloader.
     * Receive and send loops should be called after that.
     */
    void Start();

    /*
     * Terminates the downloader.
     */
    void Terminate();
    
    /*
     * Message receive loop.
     */
    void ReceiveLoop();

    /*
     * Message send loop.
     */
    void SendLoop();

    /*
     * Add specified piece to downloader's pending pieces queue.
     */
    void QueuePiece(std::shared_ptr<Piece> piece);

    /*
     * Cancel request for the specified piece.
     */
    void CancelPiece(std::shared_ptr<Piece> piece);

    /*
     * Returns amount of pieces that are currently waiting to be requested
     * from peer.
     */
    size_t GetQueuedPiecesCount() const;

    /*
     * Returns amount of pieces were requested and are waiting to be received
     * from peer.
     */
    size_t GetRequestedPiecesCount() const;
    
    /*
     * Checks if the peer has informed that it has piece with specified index.
     */
    bool IsPieceAvailable(size_t pieceIdx);

    /*
     * Returns true if downloader is running and false otherwise.
     */
    bool IsRunning() const;
    
    /*
     * Clear pieces queue and discard all requested pieces.
     * If `sendCancel` is true, also send `cancel` messages
     * for requested pieces (if possible).
     */
    void FlushPieces(bool sendCancel = false);
private:
    void RequestBlocksForPiece(std::shared_ptr<Piece> piece);
    void CancelBlocksForPiece(std::shared_ptr<Piece> piece);

    std::shared_ptr<Piece> AcquirePiece();

    void ProcessChokeMessage(Message& msg);
    void ProcessUnchokeMessage(Message& msg);
    // void ProcessInterestedMessage(Message& msg);
    // void ProcessNotInterestedMessage(Message& msg);
    void ProcessHaveMessage(Message& msg);
    void ProcessBitFieldMessage(Message& msg);
    // void ProcessRequestMessage(Message& msg);
    void ProcessPieceMessage(Message& msg);
    // void ProcessCancelMessage(Message& msg);
    

    mutable std::mutex queueMtx_, reqMtx_, availabilityMtx_;
    PeerConnection connection_;

    std::vector<std::shared_ptr<Piece>> queuedPieces_;
    std::vector<std::shared_ptr<Piece>> requestedPieces_;

    std::function<void(std::shared_ptr<Piece>)> downloadCallback_;
    std::vector<bool> pieceAvailability_;

    std::atomic<int> requestedLimit_ = 2;
    std::atomic<bool> choked_ = true;
    // std::atomic<bool> interested_ = false;
};