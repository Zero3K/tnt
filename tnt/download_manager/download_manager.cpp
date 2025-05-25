#include "download_manager.h"
#include "../tcp_connection/exception.h"
#include <memory>
#include <iostream>
#include <utility>
#include <thread>
#include <arpa/inet.h>

using namespace std::chrono_literals;

DownloadManager::DownloadManager(Peer peer, PieceStorage& storage, std::string hash) : 
        storage_(&storage), con_(peer, "TTST1APP2DONT3WORRY6", hash) {

}

void DownloadManager::EstablishConnection() {
    con_.EstablishConnection();
    // std::cout << "Connection established!" << std::endl;
}

void DownloadManager::RequestBlocksForPiece(std::shared_ptr<Piece> piece) {
    for (size_t j = 0; j < piece->GetBlocks().size(); j++) {
        auto& block = piece->GetBlocks()[j];
        con_.SendMessage(Message::InitRequest(piece->GetIndex(), block.offset, block.length));
    }
}

void DownloadManager::SendLoop() {
    recvRunning_ = true;
    con_.SendMessage(Message::Init(Message::Id::Interested));
    while (!storage_->AllPiecesDownloaded() && !terminating_) {
        if (!choked_)
            while (requestedPieces_.size() < 15 && !storage_->AllPiecesDownloaded()) {
                auto piece = storage_->AcquirePiece();
                if (piece != nullptr) {
                    RequestBlocksForPiece(piece);
                    requestedPieces_[piece->GetIndex()] = piece;
                }
            }

        std::this_thread::sleep_for(15ms);
    }

    sendRunning_ = false;
    sendRunning_.notify_one();
}

void DownloadManager::ReceiveLoop() {
    recvRunning_ = true;

    try {
        while (!storage_->AllPiecesDownloaded() && !terminating_) {
            Message msg = con_.RecieveMessage();
            if (msg.id == Message::Id::Unchoke) {
                choked_ = false;
                choked_.notify_one();
            } else if (msg.id == Message::Id::Piece) {
                uint32_t pieceIndex = ntohl(*reinterpret_cast<const uint32_t*>(&msg.payload[0]));
                uint32_t blockOffset = ntohl(*reinterpret_cast<const uint32_t*>(&msg.payload[4]));
                std::string data = msg.payload.substr(8);

                auto it = requestedPieces_.find(pieceIndex);
                if (it == requestedPieces_.end())
                    continue;
                auto piece = it->second;
                piece->SaveBlock(blockOffset, data);

                if (piece->AllBlocksRetrieved()) {
                    requestedPieces_.erase(pieceIndex);
                    if (piece->Validate())
                        storage_->PieceDownloaded(piece);
                }
            }
        }
    } catch (std::runtime_error& exc) {
        recvRunning_ = false;
        recvRunning_.notify_one();
        throw exc;
    }

    recvRunning_ = false;
    recvRunning_.notify_one();
}

void DownloadManager::Terminate() {
    if (terminating_)
        return;
    terminating_ = true;
    // sendRunning_.wait(true);
    // recvRunning_.wait(true);
    // std::cout << "terminated!\n";
    con_.CloseConnection();
}