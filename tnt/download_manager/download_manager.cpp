#include "download_manager.h"
#include <memory>
#include <iostream>
#include <utility>
#include <arpa/inet.h>

using namespace std::chrono_literals;

DownloadManager::DownloadManager(ThreadPool& threadPool, std::vector<Peer> peers, PieceStorage& storage, std::string hash) : storage_(storage), threadPool_(threadPool) {
    cons_.reserve(peers.size());
    for (size_t i = 0; i < peers.size(); i++) {
        cons_.emplace_back(peers[i], "TEST0APP1DONT2WORRY3", hash);
    }
}

void DownloadManager::EstablishConnections() {
    std::atomic<int> cnt = cons_.size();
    std::vector<bool> success(cons_.size());

    auto task = [&](size_t idx) {
        try {
            cons_[idx].EstablishConnection();
            success[idx] = true;
            mtx_.lock();
            std::cout << "Connection with " << idx << " successful!" << std::endl;
            mtx_.unlock();
        } catch (...) {
            success[idx] = false;
            mtx_.lock();
            std::cout << "Connection with " << idx << " failed :(" << std::endl;
            mtx_.unlock();
        }

        cnt--;
        if (cnt == 0)
            cnt.notify_one();
    };

    for (size_t i = 0; i < cons_.size(); i++) {
        threadPool_.PushTask([i, &task]() { task(i); });
    }

    int value = cnt.load();
    if (value != 0) {
        cnt.wait(value);
    }

    for (size_t i = 0; i < cons_.size(); i++) {
        if (success[i]) {
            availableCons_.insert(&cons_[i]);
        }
    }

    std::cout << "Made " << availableCons_.size() << " successful connections in total" << std::endl;
}

void DownloadManager::Run() {
    EstablishConnections();
    StartDownloading();
}

void DownloadManager::RequestBlocksForPiece(PeerConnection& con, std::shared_ptr<Piece> piece) {
    for (size_t j = 0; j < piece->GetBlocks().size(); j++) {
        auto& block = piece->GetBlocks()[j];

        std::string payload(12, 0);
        *reinterpret_cast<uint32_t*>(&payload[0]) = htonl(static_cast<uint32_t>(piece->GetIndex()));
        *reinterpret_cast<uint32_t*>(&payload[4]) = htonl(static_cast<uint32_t>(block.offset));
        *reinterpret_cast<uint32_t*>(&payload[8]) = htonl(static_cast<uint32_t>(block.length));

        con.SendMessage(Message::Init(Message::Id::Request, payload));
    }
}

void DownloadManager::DownloadLoop() {
    std::unique_lock lock(mtx_);
    if (availableCons_.empty())
        return;

    PeerConnection* conPtr = *availableCons_.begin(); 
    availableCons_.erase(conPtr);
    std::cout << "Starting download from thread " << std::this_thread::get_id() << std::endl;
    lock.unlock();

    bool choked = true;

    std::vector<size_t> requested;
    while (!storage_.AllPiecesGood()) {
        if (requested.empty() && !choked) {
            for (size_t i = 0; i < 15; i++) {
                auto piece = storage_.GetNextPieceToDownload();
                if (piece != nullptr) {
                    requested.push_back(piece->GetIndex());
                    RequestBlocksForPiece(*conPtr, piece);
                }
            }
        } else {
            Message msg = conPtr->RecieveMessage();
            if (msg.id == Message::Id::Unchoke) {
                choked = false;  
            } else if (msg.id == Message::Id::Piece) {
                uint32_t pieceIndex = ntohl(*reinterpret_cast<const uint32_t*>(&msg.payload[0]));
                uint32_t blockOffset = ntohl(*reinterpret_cast<const uint32_t*>(&msg.payload[4]));
                std::string data = msg.payload.substr(8);

                if (std::find(requested.begin(), requested.end(), pieceIndex) == requested.end())
                    continue;

                auto piece = storage_.GetPiece(pieceIndex);
                storage_.GetPiece(pieceIndex)->SaveBlock(blockOffset, data);

                if (piece->Validate()) {
                    storage_.PieceProcessed(piece);
                    requested.erase(std::find(requested.begin(), requested.end(), pieceIndex));
                } else {
                    storage_.PieceFailed(piece);
                }
            }
        }
    }
}

void DownloadManager::StartDownloading() {
    for (size_t i = 0; i < availableCons_.size(); i++) {
        threadPool_.PushTask([&]() {
            try {
                DownloadLoop();
            } catch (std::runtime_error exc) {
                mtx_.lock();
                std::cout << "Some peer disconnected, reason: " << exc.what() << std::endl;
                mtx_.unlock();
            }
        });
    }

    while (!storage_.AllPiecesGood()) {
        std::this_thread::sleep_for(1s);
    }
}