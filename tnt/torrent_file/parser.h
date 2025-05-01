#pragma once

#include <iostream>
#include "types.h"

namespace TorrentFile {
    Metainfo ParseTorrentFile(std::istream& stream);
    std::istream& operator>>(std::istream&, Metainfo&);
}