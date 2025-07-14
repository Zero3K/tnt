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
     * Adds specified piece to downloader's pending pieces queue.
     */
    void QueuePiece(std::shared_ptr<Piece> piece);

    /*
     * Removes specified piece from downloader's pending pieces queue.
     * This will do nothing if piece has already been requested.
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
private:
    void RequestBlocksForPiece(std::shared_ptr<Piece> piece);
    std::shared_ptr<Piece> AcquirePiece();

    void ProcessHaveMessage(Message& msg);
    void ProcessPieceMessage(Message& msg);
    void ProcessChokeMessage(Message& msg);
    void ProcessUnchokeMessage(Message& msg);
    void ProcessBitfieldMessage(Message& msg);

    mutable std::mutex queueMtx_, reqMtx_, availabilityMtx_;
    PeerConnection connection_;

    std::vector<std::shared_ptr<Piece>> queuedPieces_;
    std::vector<std::shared_ptr<Piece>> requestedPieces_;

    std::function<void(std::shared_ptr<Piece>)> downloadCallback_;
    std::vector<bool> pieceAvailability_;

    std::atomic<bool> choked_ = true;
};