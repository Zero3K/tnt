#include "torrent_tracker.h"
#include "../bencode/decoding.h"
#include <cpr/cpr.h>
#include <sstream>
#include <netinet/in.h>

#include <iostream>


namespace TorrentTracker {
    Tracker::Tracker(const std::string& url) : url_(url) {

    }

    void Tracker::UpdatePeers(const TorrentFile::Metainfo& tf, std::string peerId, int port) {
        cpr::Response response = cpr::Get(
            cpr::Url { tf.announce },
            cpr::Parameters {
                { "info_hash", tf.infoHash },
                { "peer_id", peerId },
                { "port", std::to_string(port) },
                { "uploaded", std::to_string(port) },
                { "downloaded", std::to_string(port) },
                { "left", std::to_string(tf.length) },
                { "compact", std::to_string(1) }
            }
        );

        std::stringstream stream;
        stream << response.text;

        auto dataDict = std::static_pointer_cast<Bencode::Dict>(Bencode::ReadEntity(stream))->value;
        auto rawPeers = std::static_pointer_cast<Bencode::String>(dataDict["peers"])->value;

        peers_.clear();

        for (size_t i = 0; i < rawPeers.size(); i += 6) {
            peers_.push_back(Peer {
                .ip = std::to_string((uint8_t)rawPeers[i]) + "."
                    + std::to_string((uint8_t)rawPeers[i + 1]) + "."
                    + std::to_string((uint8_t)rawPeers[i + 2]) + "."
                    + std::to_string((uint8_t)rawPeers[i + 3]),
                .port = htons(*reinterpret_cast<uint16_t*>(&rawPeers[i + 4]))
            });
        }
    }

    const std::vector<Peer>& Tracker::GetPeers() const {
        return peers_;
    }
}