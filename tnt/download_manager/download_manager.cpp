#include "download_manager.h"
#include <memory>
#include <iostream>

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

    int successCount = 0;
    for (int i = 0; i < cons_.size(); i++)
        successCount += success[i];

    std::cout << "Made " << successCount << " successful connections in total" << std::endl;
}

void DownloadManager::Run() {
    EstablishConnections();
}