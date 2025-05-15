#include "peer_connection.h"
#include "tcp_connection/exception.h"
#include "message.h"
#include "piece_storage/piece_storage.h"
#include "torrent_file/types.h"
#include <iostream>
#include <sys/wait.h>
#include <netinet/in.h>
#include <optional>
#include <chrono>


using namespace std::chrono_literals;


PeerConnection::PeerConnection(const Peer& peer, std::string selfId, std::string hash) 
    : peer_(peer), selfId_(selfId), hash_(hash), socket_(peer.ip, peer.port) {}

PeerConnection::~PeerConnection() {
    socket_.CloseConnection();
}

void PeerConnection::PerformHandshake() {    
    std::string proto = "BitTorrent protocol";

    std::string data;
    data.reserve(49 + proto.size());

    data += char(proto.size());
    data += proto;
    data.resize(8 + data.size());

    data += hash_;
    data += selfId_;
    
    socket_.EstablishConnection();
    socket_.SendData(data);

    int peerProtoSize = static_cast<int>(socket_.ReceiveData(1)[0]);
    if (peerProtoSize == 0) {
        throw std::runtime_error("bad peer proto size");
    }
    std::string peerProto = socket_.ReceiveData(peerProtoSize);
    std::string suf = socket_.ReceiveData(48);

    peerId_ = suf.substr(28);
}

void PeerConnection::EstablishConnection() {
    socket_.EstablishConnection();
    PerformHandshake();
}

void PeerConnection::SendMessage(const Message& msg) const {
    socket_.SendData(msg.ToString());
}

std::optional<Message> PeerConnection::RecieveMessage() const {
    try {
        Message msg = Message::Parse(socket_.ReceiveData());
        return std::make_optional(msg);
    } catch (TcpTimeoutError& exc) {
        return std::nullopt;
    } catch (std::exception& exc) {
        throw exc;
    } catch (...) {
        throw std::runtime_error("Unknown error");
    }
}

void PeerConnection::CloseConnection() {
    socket_.CloseConnection();
}