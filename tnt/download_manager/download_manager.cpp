#include "download_manager.h"
#include "../tcp_connection/exception.h"
#include <memory>
#include <iostream>
#include <utility>
#include <thread>
#include <arpa/inet.h>

using namespace std::chrono_literals;

DownloadManager::DownloadManager(Peer peer, PieceStorage& storage, std::string hash) : storage_(&storage), con_(peer, "TEST1APP2DONT3WORRY6", hash) {

}

void DownloadManager::EstablishConnection() {
    con_.EstablishConnection();
    // std::cout << "Connection established!" << std::endl;
}

void DownloadManager::RequestBlocksForPiece(std::shared_ptr<Piece> piece) {
    for (size_t j = 0; j < piece->GetBlocks().size(); j++) {
        auto& block = piece->GetBlocks()[j];

        std::string payload(12, 0);
        *reinterpret_cast<uint32_t*>(&payload[0]) = htonl(static_cast<uint32_t>(piece->GetIndex()));
        *reinterpret_cast<uint32_t*>(&payload[4]) = htonl(static_cast<uint32_t>(block.offset));
        *reinterpret_cast<uint32_t*>(&payload[8]) = htonl(static_cast<uint32_t>(block.length));

        con_.SendMessage(Message::Init(Message::Id::Request, payload));
    }
}

void DownloadManager::SendLoop() {
    recvRunning_ = true;
    while (!storage_->AllPiecesGood() && !terminating_) {
        if (!choked_)
            while (requestedPieces_.size() < 15 && !storage_->AllPiecesGood()) {
                auto piece = storage_->AcquirePiece();
                if (piece != nullptr) {
                    RequestBlocksForPiece(piece);
                    requestedPieces_.insert(piece);
                }
            }

        std::this_thread::sleep_for(15ms);
    }

    sendRunning_ = false;
    sendRunning_.notify_one();
}

void DownloadManager::ReceiveLoop() {
    recvRunning_ = true;
    while (!storage_->AllPiecesGood() && !terminating_) {
        Message msg = con_.RecieveMessage();

        if (msg.id == Message::Id::Unchoke) {
            choked_ = false;
            choked_.notify_one();
        } else if (msg.id == Message::Id::Piece) {
            uint32_t pieceIndex = ntohl(*reinterpret_cast<const uint32_t*>(&msg.payload[0]));
            uint32_t blockOffset = ntohl(*reinterpret_cast<const uint32_t*>(&msg.payload[4]));
            std::string data = msg.payload.substr(8);

            auto piece = storage_->GetPiece(pieceIndex);
            storage_->GetPiece(pieceIndex)->SaveBlock(blockOffset, data);

            requestedPieces_.erase(piece);
            if (piece->Validate()) {
                storage_->PieceProcessed(piece);
            }
        }
    }

    recvRunning_ = false;
    recvRunning_.notify_one();
}

void DownloadManager::Terminate() {
    terminating_ = true;
    sendRunning_.wait(true);
    recvRunning_.wait(true);
    // std::cout << "terminated!\n";
    con_.CloseConnection();
}