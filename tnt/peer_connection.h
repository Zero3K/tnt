#pragma once

#include "piece_storage/piece_storage.h"
#include "tcp_connection/tcp_connection.h"
#include "torrent_tracker.h"
#include "torrent_file/types.h"
#include "message.h"
#include <memory>


class PeerConnection {
public:
    PeerConnection(const Peer& peer, std::string selfId, std::string hash);
    ~PeerConnection();

    // Establish connection to peer.
    void EstablishConnection();

    // Send message to a peer.
    void SendMessage(const Message& msg) const;

    // Recieve message from a peer.
    std::optional<Message> RecieveMessage() const;

    // Close connection to peer.
    void CloseConnection();
private:
    const Peer& peer_;
    std::string selfId_, peerId_, hash_;
    TcpConnection socket_;

    // Perfrom handshake. Should be called first after establishing connection.
    void PerformHandshake();
};