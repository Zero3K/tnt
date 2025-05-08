#include "peer_connection.h"
#include "message.h"
#include <iostream>
#include <sstream>
#include <utility>
#include "../bencode/types.h"
#include "../tcp_connection/tcp_connection.h"
#include "../torrent_file/types.h"
#include "../torrent_tracker/torrent_tracker.h"
#include <netinet/in.h>

using namespace std::chrono_literals;

PeerConnect::PeerConnect(const TorrentTracker::Peer& peer, const TorrentFile::Metainfo &tf, std::string selfPeerId) :
    tf_(tf), socket_(peer.ip, peer.port, 1000ms, 5000ms), selfPeerId_(selfPeerId) {   
}

void PeerConnect::Run() {
    while (!terminated_) {
        if (EstablishConnection()) {
            std::cout << "Connection established to peer" << std::endl;
            MainLoop();
        } else {
            std::cerr << "Cannot establish connection to peer" << std::endl;
            Terminate();
        }
    }
}

void PeerConnect::PerformHandshake() {
    std::string data;
    
    std::string proto = "BitTorrent protocol";
    data.reserve(49 + proto.size());

    data += char(proto.size());
    data += proto;
    data.resize(8 + data.size());

    data += tf_.infoHash;
    data += selfPeerId_;
    
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

bool PeerConnect::EstablishConnection() {
    try {
        PerformHandshake();
        ReceiveBitfield();
        SendInterested();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to establish connection with peer " << std::endl;
        return false;
    }
}

void PeerConnect::ReceiveBitfield() {
    Message msg = Message::Parse(socket_.ReceiveData());

    if (msg.id == MessageId::BitField) {
        piecesAvailability_ = PeerPiecesAvailability(msg.payload);
        // std::cout << "size: " << suf.substr(1).size() * 8 << " expect " << (tf_.pieceHashes.size() + 7) / 8 * 8 << std::endl;;
    } else if (msg.id == MessageId::Unchoke) {
        choked_ = false;
        std::string bf;
        bf.resize((tf_.pieceHashes.size() + 7) / 8 * 8);
        piecesAvailability_ = PeerPiecesAvailability(bf);
    } else {
        throw std::runtime_error("bad peer");
    }
}

void PeerConnect::SendInterested() {
    socket_.SendData(Message::Init(MessageId::Interested, "").ToString());
}

void PeerConnect::Terminate() {
    std::cerr << "Terminate" << std::endl;
    terminated_ = true;
}

void PeerConnect::MainLoop() {
    std::cout << "Dummy main loop" << std::endl;

    // Message msg = socket_.ReceiveData();
    // if (!data.empty())
    //     std::cout << "got data, id = " << static_cast<int>(data[0]) << '\n'; 
    // data = socket_.ReceiveData();
    // if (!data.empty())
    //     std::cout << "got data, id = " << static_cast<int>(data[0]) << '\n'; 
    // data = socket_.ReceiveData();
    // if (!data.empty())
    //     std::cout << "got data, id = " << static_cast<int>(data[0]) << '\n'; 

    Terminate();
}
