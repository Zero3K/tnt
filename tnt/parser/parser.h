#include <string>
#include <vector>
#include <openssl/sha.h>
#include <fstream>
#include <map>
#include <memory>


struct TorrentFile {
    std::string announce;
    std::string comment;
    std::vector<std::string> pieceHashes;
    size_t pieceLength;
    size_t length;
    std::string name;
    std::string infoHash;
};

struct TfEntity {
    virtual std::string Encode() const = 0;
    virtual ~TfEntity() = default;
};

std::shared_ptr<TfEntity> ReadEntity(std::ifstream& stream);

struct TfInteger : public TfEntity {
    int64_t value;
    
    std::string Encode() const override;
    static std::shared_ptr<TfInteger> Read(std::ifstream& stream);
};

struct TfString : public TfEntity {
    std::string value;

    std::string Encode() const override;
    static std::shared_ptr<TfString> Read(std::ifstream& stream);
};

struct TfList : public TfEntity {
    std::vector<std::shared_ptr<TfEntity>> contents;

    std::string Encode() const override;
    static std::shared_ptr<TfList> Read(std::ifstream& stream);
};

struct TfDict : public TfEntity {
    std::map<std::string, std::shared_ptr<TfEntity>> contents;

    std::string Encode() const override;
    static std::shared_ptr<TfDict> Read(std::ifstream& stream);
};

std::shared_ptr<TfEntity> ReadEntity(std::ifstream& stream);

TorrentFile LoadTorrentFile(const std::string& filename);
