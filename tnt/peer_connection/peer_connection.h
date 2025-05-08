#pragma once

#include "../tcp_connection/tcp_connection.h"
#include "../torrent_file/types.h"
#include "../torrent_tracker/torrent_tracker.h"

class PeerPiecesAvailability {
public:
    PeerPiecesAvailability() {
        
    }

    explicit PeerPiecesAvailability(std::string bitfield) : bitfield_(bitfield) {}

    bool IsPieceAvailable(size_t pieceIndex) const {
        return (bitfield_[pieceIndex / 8] >> (pieceIndex % 8)) & 1;
    }

    void SetPieceAvailability(size_t pieceIndex) {
        bitfield_[pieceIndex / 8] |= (1 << (pieceIndex % 8));
    }

    size_t Size() const {
        return bitfield_.size() * 8;
    }
private:
    std::string bitfield_;
};

/*
 * Класс, представляющий соединение с одним пиром.
 * С помощью него можно подключиться к пиру и обмениваться с ним сообщениями
 */
class PeerConnect {
public:
    PeerConnect(const TorrentTracker::Peer& peer, const TorrentFile::Metainfo& tf, std::string selfPeerId);

    /*
     * Основная функция, в которой будет происходить цикл общения с пиром.
     * https://wiki.theory.org/BitTorrentSpecification#Messages
     */
    void Run();

    void Terminate();
protected:
    const TorrentFile::Metainfo& tf_;
    TcpConnect socket_;  // tcp-соединение с пиром
    const std::string selfPeerId_;  // наш id, которым представляется наш клиент
    std::string peerId_;  // id пира, с которым мы общаемся в текущем соединении
    PeerPiecesAvailability piecesAvailability_;
    bool terminated_ = false;  // флаг, необходимый для завершения цикла общения с пиром
    bool choked_ = true;  // https://wiki.theory.org/BitTorrentSpecification#Overview

    /*
     * Функция производит handshake.
     * - Подключиться к пиру по протоколу TCP
     * - Отправить пиру сообщение handshake
     * - Проверить правильность ответа пира
     * https://wiki.theory.org/BitTorrentSpecification#Handshake
     */
    void PerformHandshake();

    /*
     * - Провести handshake
     * - Получить bitfield с информацией о наличии у пира различных частей файла
     * - Сообщить пиру, что мы готовы получать от него данные (отправить interested)
     * Возвращает true, если все три этапа прошли без ошибок
     */
    bool EstablishConnection();

    /*
     * Функция читает из сокета bitfield с информацией о наличии у пира различных частей файла.
     * Полученную информацию надо сохранить в поле `piecesAvailability_`.
     * Также надо учесть, что сообщение тип Bitfield является опциональным, то есть пиры необязательно будут слать его.
     * Вместо этого они могут сразу прислать сообщение Unchoke, поэтому надо быть готовым обработать его в этой функции.
     * Обработка сообщения Unchoke заключается в выставлении флага `choked_` в значение `false`
     */
    void ReceiveBitfield();

    /*
     * Функция посылает пиру сообщение типа interested
     */
    void SendInterested();

    virtual void MainLoop();
};
