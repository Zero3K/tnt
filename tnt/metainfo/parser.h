#include <iostream>
#include "types.h"

namespace Metainfo {
    TorrentFile Parse(std::istream& stream);
    std::istream& operator>>(std::istream&, TorrentFile&);
}