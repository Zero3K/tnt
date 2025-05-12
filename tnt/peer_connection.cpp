#include "peer_connection.h"
#include "message.h"
#include "piece_storage/piece_storage.h"
#include "torrent_file/types.h"
#include <iostream>
#include <sys/wait.h>
#include <netinet/in.h>


using namespace std::chrono_literals;

PeerPiecesAvailability::PeerPiecesAvailability() = default;

PeerPiecesAvailability::PeerPiecesAvailability(std::string bitfield) : bitfield_(bitfield) {}

bool PeerPiecesAvailability::IsPieceAvailable(size_t pieceIndex) const {
    return (bitfield_[pieceIndex / 8] >> (pieceIndex % 8)) & 1;
}

void PeerPiecesAvailability::SetPieceAvailability(size_t pieceIndex) {
    bitfield_[pieceIndex / 8] |= (1 << (pieceIndex % 8));
}

PeerConnection::PeerConnection(const Peer& peer, const TorrentFile &tf, std::string selfPeerId, PieceStorage& pieceStorage) :
        tf_(tf), socket_(peer.ip, peer.port, 500ms, 500ms), selfPeerId_(selfPeerId), pieceStorage_(pieceStorage) {
    piecesAvailability_ = PeerPiecesAvailability(std::string((tf_.pieceHashes.size() + 7) / 8 * 8, 0)); 
}

void PeerConnection::Run() {
    EstablishConnection();
    RunMessageFlow();
}

void PeerConnection::PerformHandshake() {
    std::string data;
    
    std::string proto = "BitTorrent protocol";
    data.reserve(49 + proto.size());

    data += char(proto.size());
    data += proto;
    data.resize(8 + data.size());

    data += tf_.infoHash;
    data += selfPeerId_;
    
    socket_.EstablishConnection();
    socket_.SendData(data);

    int peerProtoSize = static_cast<int>(socket_.ReceiveData(1)[0]);
    if (peerProtoSize == 0) {
        throw std::runtime_error("bad peer proto size");
    }
    std::string peerProto = socket_.ReceiveData(peerProtoSize);
    std::string suf = socket_.ReceiveData(48);

    peerId_ = suf.substr(28);
}

void PeerConnection::EstablishConnection() {
    socket_.EstablishConnection();
    PerformHandshake();
}

void PeerConnection::SendKeepAliveMsg() {
    socket_.SendData(Message::Init(Message::Id::KeepAlive).ToString());
}

void PeerConnection::SendChokeMsg() {
    amChoking_ = true;
    socket_.SendData(Message::Init(Message::Id::Choke).ToString());
}

void PeerConnection::SendUnchokeMsg() {
    amChoking_ = false;
    socket_.SendData(Message::Init(Message::Id::Unchoke).ToString());
}

void PeerConnection::SendInterestedMsg() {
    amInterested_ = true;
    socket_.SendData(Message::Init(Message::Id::Interested).ToString());
}

void PeerConnection::SendNotInterestedMsg() {
    amInterested_ = false;
    socket_.SendData(Message::Init(Message::Id::NotInterested).ToString());
}

void PeerConnection::SendRequestMsg(size_t pieceIndex, size_t blockOffset, size_t blockLength) {
    std::string payload(12, 0);
    *reinterpret_cast<uint32_t*>(&payload[0]) = htonl(static_cast<uint32_t>(pieceIndex));
    *reinterpret_cast<uint32_t*>(&payload[4]) = htonl(static_cast<uint32_t>(blockOffset));
    *reinterpret_cast<uint32_t*>(&payload[8]) = htonl(static_cast<uint32_t>(blockLength));
    socket_.SendData(Message::Init(Message::Id::Request, payload).ToString());
}

void PeerConnection::ProcessMsg(const Message& msg) {
    switch (msg.id) {
        case Message::Id::KeepAlive:
            ProcessKeepAliveMsg(msg);
            return;
        case Message::Id::Choke:
            ProcessChokeMsg(msg);
            return;
        case Message::Id::Unchoke:
            ProcessUnchokeMsg(msg);
            return;
        case Message::Id::Interested:
            ProcessInterestedMsg(msg);
            return;
        case Message::Id::NotInterested:
            ProcessNotInterestedMsg(msg);
            return;
        case Message::Id::BitField:
            ProcessBitfieldMsg(msg);
            return;
        case Message::Id::Piece:
            ProcessPieceMsg(msg);
            return;
        default:
            return;
    }
}

void PeerConnection::ProcessKeepAliveMsg(const Message& msg) {
    // i guess we just ignore that
}

void PeerConnection::ProcessChokeMsg(const Message& msg) {
    peerChoking_ = true;
}

void PeerConnection::ProcessUnchokeMsg(const Message& msg) {
    peerChoking_ = false;
}

void PeerConnection::ProcessInterestedMsg(const Message& msg) {
    peerInterested_ = true;
}

void PeerConnection::ProcessNotInterestedMsg(const Message& msg) {
    peerInterested_ = false;
}

void PeerConnection::ProcessBitfieldMsg(const Message& msg) {
    piecesAvailability_ = PeerPiecesAvailability(msg.payload);
}

void PeerConnection::ProcessPieceMsg(const Message& msg) {
    uint32_t pieceIndex = ntohl(*reinterpret_cast<const uint32_t*>(&msg.payload[0]));
    uint32_t blockOffset = ntohl(*reinterpret_cast<const uint32_t*>(&msg.payload[4]));
    std::string data = msg.payload.substr(8);

    waitingBlock_ = false;
    currentPiece_->SaveBlock(blockOffset, data);

    if (currentPiece_->Validate()) {
        pieceStorage_.PieceProcessed(currentPiece_);
        currentPiece_ = nullptr;
    }
}

void PeerConnection::Terminate() {
    terminated_ = true;
}

void PeerConnection::RunMessageFlow() {
    while (!terminated_) {
        if (!waitingBlock_ && !peerChoking_) {
            if (currentPiece_ == nullptr) {
                if (pieceStorage_.QueueIsEmpty()) {
                    Terminate();
                    return;
                } else {
                    currentPiece_ = pieceStorage_.GetNextPieceToDownload();
                }
            }

            auto &block = currentPiece_->GetFirstMissingBlock();
            SendRequestMsg(currentPiece_->GetIndex(), block.offset, block.length);
            waitingBlock_ = true;
        } else {
            Message msg = Message::Parse(socket_.ReceiveData());
            ProcessMsg(msg);
        }
    }
}
