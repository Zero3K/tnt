#include "download_manager.h"
#include <memory>
#include <iostream>
#include <utility>
#include <arpa/inet.h>

using namespace std::chrono_literals;

DownloadManager::DownloadManager(Peer peer, PieceStorage& storage, std::string hash) : storage_(storage), con_(peer, "TEST1APP2DONT3WORRY4", hash) {

}

void DownloadManager::EstablishConnection() {
    con_.EstablishConnection();
    std::cout << "Connection established!" << std::endl;
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
    std::cout << "started send! " << "\n";
    while (!storage_.AllPiecesGood() && !terminated_) {
        choked_.wait(true);
        while (requestedPieces_.empty()) {
            for (int i = 0; i < 15; i++) {
                auto piece = storage_.GetNextPieceToDownload();
                if (piece != nullptr) {
                    RequestBlocksForPiece(piece);
                    requestedPieces_.insert(piece);
                }
            }
        }

        std::this_thread::sleep_for(15ms);
    }
    std::cout << "ended send\n";
}

void DownloadManager::ReceiveLoop() {
    std::cout << "started recv! " << "\n";
    while (!storage_.AllPiecesGood() && !terminated_) {
        Message msg = con_.RecieveMessage();
        // std::cout << "got " << int(mcsg.id) << std::endl;
        if (msg.id == Message::Id::Unchoke) {
            choked_ = false;
            choked_.notify_one();
        } else if (msg.id == Message::Id::Piece) {
            uint32_t pieceIndex = ntohl(*reinterpret_cast<const uint32_t*>(&msg.payload[0]));
            uint32_t blockOffset = ntohl(*reinterpret_cast<const uint32_t*>(&msg.payload[4]));
            std::string data = msg.payload.substr(8);

            auto piece = storage_.GetPiece(pieceIndex);
            storage_.GetPiece(pieceIndex)->SaveBlock(blockOffset, data);

            if (piece->Validate()) {
                storage_.PieceProcessed(piece);
                requestedPieces_.erase(piece);
            } else {
                storage_.PieceFailed(piece);
            }
        }
    }
}

void DownloadManager::Terminate() {
    con_.CloseConnection();
    terminated_ = true;
}