#pragma once

#include "piece_storage/piece_storage.h"
#include "tcp_connection.h"
#include "torrent_tracker.h"
#include "torrent_file/types.h"
#include "message.h"
#include <memory>


class PeerPiecesAvailability {
public:
    PeerPiecesAvailability();

    explicit PeerPiecesAvailability(std::string bitfield);

    bool IsPieceAvailable(size_t pieceIndex) const;

    void SetPieceAvailability(size_t pieceIndex);
private:
    std::string bitfield_;
};


class PeerConnection {
public:
    PeerConnection(const Peer& peer, const TorrentFile& tf, std::string selfPeerId, PieceStorage& pieceStorage);

    void Run();

    void Terminate();
    
private:
    const TorrentFile& tf_;
    TcpConnection socket_;
    const std::string selfPeerId_;
    std::string peerId_;
    PeerPiecesAvailability piecesAvailability_;

    PieceStorage& pieceStorage_;
    std::shared_ptr<Piece> currentPiece_;

    bool peerChoking_ = true;
    bool peerInterested_ = false;
    bool amChoking_ = true;
    bool amInterested_ = false;

    bool terminated_ = false;
    bool waitingBlock_ = false;

    void PerformHandshake();
    void EstablishConnection();

    void SendKeepAliveMsg();
    void SendChokeMsg();
    void SendUnchokeMsg();
    void SendInterestedMsg();
    void SendNotInterestedMsg();
    void SendRequestMsg(size_t pieceIndex, size_t blockOffset, size_t blockLength);

    void ProcessMsg(const Message& msg);
    void ProcessKeepAliveMsg(const Message& msg);
    void ProcessChokeMsg(const Message& msg);
    void ProcessUnchokeMsg(const Message& msg);
    void ProcessInterestedMsg(const Message& msg);
    void ProcessNotInterestedMsg(const Message& msg);
    void ProcessBitfieldMsg(const Message& msg);
    void ProcessPieceMsg(const Message& msg);

    void RequestPiece();

    void RunMessageFlow();
};
