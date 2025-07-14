#include "peer_downloader.h"
#include "../torrent_file/types.h"
#include <memory>
#include <iostream>
#include <utility>
#include <thread>
#include <arpa/inet.h>

using namespace std::chrono_literals;
const int oneTimeRequestedLimit = 5;

PeerDownloader::PeerDownloader(Peer peer, TorrentFile file,
        std::function<void(std::shared_ptr<Piece>)> callback) : 
    connection_(peer, "TTST1APP2DONT3WORRY6", file.infoHash),
    pieceAvailability_(file.pieceHashes.size(), false),
    downloadCallback_(callback) {}

void PeerDownloader::QueuePiece(std::shared_ptr<Piece> piece) {
    queueMtx_.lock();
    queuedPieces_.push_back(piece);
    queueMtx_.unlock();
}

void PeerDownloader::CancelPiece(std::shared_ptr<Piece> piece) {
    queueMtx_.lock();
    std::erase_if(queuedPieces_, [&](const std::shared_ptr<Piece>& ptr) { 
        return ptr->GetIndex() == piece->GetIndex();
    });
    queueMtx_.unlock();
}

bool PeerDownloader::IsPieceAvailable(size_t pieceIdx) {
    std::lock_guard lock(availabilityMtx_);
    return pieceAvailability_[pieceIdx];
}

void PeerDownloader::Start() {
    connection_.EstablishConnection();
    connection_.SendMessage(Message::Init(Message::Id::Interested));
}

size_t PeerDownloader::GetQueuedPiecesCount() const {
    std::unique_lock lock(queueMtx_);
    return queuedPieces_.size();
}

bool PeerDownloader::IsRunning() const {
    return connection_.IsRunning();
}

void PeerDownloader::RequestBlocksForPiece(std::shared_ptr<Piece> piece) {
    for (size_t j = 0; j < piece->GetBlocks().size(); j++) {
        auto& block = piece->GetBlocks()[j];
        connection_.SendMessage(Message::InitRequest(piece->GetIndex(), block.offset, block.length));
    }
}

void PeerDownloader::SendLoop() {
    while (IsRunning()) {
        if (!choked_) {
            std::unique_lock lock(reqMtx_);
            while (requestedPieces_.size() < oneTimeRequestedLimit) {
                auto piece = AcquirePiece();

                if (piece != nullptr) {
                    requestedPieces_.push_back(piece);
                    RequestBlocksForPiece(piece);
                } else break;
            }
        }

        std::this_thread::sleep_for(15ms);
    }
}

void PeerDownloader::ReceiveLoop() {
    while (IsRunning()) {
        Message msg = connection_.RecieveMessage();
        if (msg.id == Message::Id::Unchoke) {
            ProcessUnchokeMessage(msg);
        } else if (msg.id == Message::Id::Choke) {
            ProcessChokeMessage(msg);
        } else if (msg.id == Message::Id::Piece) {
            ProcessPieceMessage(msg);
        } else if (msg.id == Message::Id::BitField) {
            ProcessBitfieldMessage(msg);
        } else if (msg.id == Message::Id::Have) {
            ProcessHaveMessage(msg);
        }
    }
}

std::shared_ptr<Piece> PeerDownloader::AcquirePiece() {
    std::lock_guard lock(queueMtx_);

    if (queuedPieces_.empty())
        return nullptr;

    size_t idx = rand() % queuedPieces_.size();  // FIX
    std::swap(queuedPieces_[idx], queuedPieces_.back());
    auto piece = queuedPieces_.back();
    queuedPieces_.pop_back();

    return piece;
}

void PeerDownloader::Terminate() {
    connection_.CloseConnection();

    // canceling unfinished pieces
    queueMtx_.lock();
    while (!queuedPieces_.empty()) {
        downloadCallback_(queuedPieces_.back());
        queuedPieces_.pop_back();
    }
    queueMtx_.unlock();
    reqMtx_.lock();
    while (!requestedPieces_.empty()) {
        downloadCallback_(requestedPieces_.back());
        requestedPieces_.pop_back();
    }
    reqMtx_.unlock();

    // availabilityMtx_.lock();
    // pieceAvailability_.assign(file_)
}

// ---------------------------------------------------

void PeerDownloader::ProcessPieceMessage(Message& msg) {
    uint32_t pieceIndex = ntohl(*reinterpret_cast<const uint32_t*>(&msg.payload[0]));
    uint32_t blockOffset = ntohl(*reinterpret_cast<const uint32_t*>(&msg.payload[4]));
    std::string data = msg.payload.substr(8);

    std::lock_guard lock(reqMtx_);

    auto it = find_if(requestedPieces_.begin(), requestedPieces_.end(), [&](const std::shared_ptr<Piece>& ptr) {
        return (ptr->GetIndex() == pieceIndex);
    });
    if (it == requestedPieces_.end())
        return;

    auto piece = (*it);
    piece->SaveBlock(blockOffset, data);

    if (piece->AllBlocksRetrieved()) {
        requestedPieces_.erase(it);
        downloadCallback_(piece);
    }
}

void PeerDownloader::ProcessHaveMessage(Message& msg) {
    uint32_t pieceIndex = ntohl(*reinterpret_cast<const uint32_t*>(&msg.payload[0]));

    availabilityMtx_.lock();
    pieceAvailability_[pieceIndex] = true;
    availabilityMtx_.unlock();
}

void PeerDownloader::ProcessBitfieldMessage(Message& msg) {
    std::lock_guard lock(availabilityMtx_);

    std::string data = msg.payload;
    for (size_t i = 0; i < pieceAvailability_.size(); i++) {
        pieceAvailability_[i] = (data[i / 8] >> (i % 8)) & 1;
    }
}

void PeerDownloader::ProcessUnchokeMessage(Message& msg) {
    choked_ = false;
}

void PeerDownloader::ProcessChokeMessage(Message& msg) {
    choked_ = true;
}
