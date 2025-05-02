#pragma once

#include "../torrent_file/types.h"
#include <string>


namespace TorrentTracker {
    struct Peer {
        std::string ip;
        int port;
    };

    class Tracker {
    public:
        Tracker(const std::string& url);
        void UpdatePeers(const TorrentFile::Metainfo& tf, std::string peerId, int port);
        const std::vector<Peer>& GetPeers() const;

    private:
        std::string url_;
        std::vector<Peer> peers_;
    };
}