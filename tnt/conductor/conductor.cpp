#include "../peer_downloader/peer_downloader.h"
#include "../piece_storage/piece.h"
#include "../piece_storage/piece.h"
#include "conductor.h"
#include <set>
#include <utility>
#include <thread>
#include <iostream>

static constexpr int oneTimeQueuedLimit = 5;

Conductor::Conductor(const std::vector<Peer>& peers, const TorrentFile& file, PieceStorage& storage)
    : file_(file), peers(peers), storage_(storage) {}

Conductor::~Conductor() {
    for (auto& t : threads_)
        t.join();
}

void Conductor::ConnectPeer(Peer peer) {
    downloaders_.emplace_back(std::make_unique<PeerDownloader>(peer, file_, [&](std::shared_ptr<Piece> piece) {
        if (endgame_) {
            for (auto& ptr : downloaders_)
                ptr->CancelPiece(piece);
        }
        
        if (storage_.PieceDownloaded(piece)) {
            speedMtx_.lock();
            downloadTimes_.emplace(std::chrono::steady_clock::now());
            speedMtx_.unlock();
        }
    }));

    auto dmMainTask = [&](PeerDownloader& dwn) {
        int attempts = 0;
        auto dmReceiveTask = [&](PeerDownloader& dwn) {
            try {
                dwn.ReceiveLoop();
            } catch (std::runtime_error& exc) {
                dwn.Terminate();
            }
        };

        auto dmSendTask = [&](PeerDownloader& dwn) {
            try {
                dwn.SendLoop();
            } catch (std::runtime_error& exc) {
                dwn.Terminate();
            }
        };

        while (storage_.GetFinishedCount() < storage_.GetTotalCount()) {
            try {
                dwn.Start();
                attempts = 0;
            } catch (std::runtime_error& exc) {
                dwn.Terminate();

                attempts++;
                if (attempts > 5)
                    break;

                std::this_thread::sleep_for(1s * attempts);
                continue;
            }

            connectedPeersCount_++;

            auto t1 = std::thread(dmReceiveTask, std::ref(dwn));
            auto t2 = std::thread(dmSendTask, std::ref(dwn));

            t1.join();
            t2.join();

            dwn.Terminate();
            connectedPeersCount_--;
        }
    };
    
    threads_.emplace_back(dmMainTask, std::ref(*downloaders_.back()));
}

void Conductor::Download() {
    for (auto &peer : peers) {
        ConnectPeer(peer);
        std::this_thread::sleep_for(30ms);
    }

    while (storage_.GetFinishedCount() < storage_.GetTotalCount()) {
        if (!endgame_ && storage_.GetPendingCount() + storage_.GetFinishedCount() >= storage_.GetTotalCount()) {
            endgame_ = true;

            for (auto& ptr : downloaders_)
                ptr->FlushPieces(true);
        }

        auto piece = storage_.AcquirePiece();
        if (piece != nullptr) {
            if (endgame_) {
                BroadcastPiece(piece);    
            } else {
                QueuePiece(piece);
            }
        }
        
        // std::this_thread::sleep_for(10ms);
    }

    for (auto &dwn: downloaders_)
        dwn->Terminate();
}

void Conductor::QueuePiece(std::shared_ptr<Piece> piece) {
    while (true) {
        for (size_t i = 0; i < downloaders_.size(); i++) {
            auto &dwn = *downloaders_[i];
            if (dwn.IsRunning() && dwn.IsPieceAvailable(piece->GetIndex()) 
                    && dwn.GetQueuedPiecesCount() < oneTimeQueuedLimit) {
                dwn.QueuePiece(piece);
                return;
            }
        }
        std::this_thread::sleep_for(30ms);
    }
}

void Conductor::BroadcastPiece(std::shared_ptr<Piece> piece) {
    int cnt = 0;
    while (cnt == 0) {
        for (size_t i = 0; i < downloaders_.size(); i++) {
            auto &dwn = *downloaders_[i];
            if (dwn.IsRunning() && dwn.IsPieceAvailable(piece->GetIndex())) {
                dwn.QueuePiece(piece);
                cnt++;
            }
        }
        std::this_thread::sleep_for(30ms);
    }
}

size_t Conductor::GetConnectedCount() const {
    return connectedPeersCount_;
}

float Conductor::GetSpeed() const {
    auto now = std::chrono::steady_clock::now();
    while (!downloadTimes_.empty() && now - downloadTimes_.front() > 5s) {
        downloadTimes_.pop();
    }
    return downloadTimes_.size() / 5.0f;
}

bool Conductor::isEndgame() {
    return endgame_;
}