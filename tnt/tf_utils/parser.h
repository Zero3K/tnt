#pragma once

#include <iostream>
#include "types.h"

namespace TFUtils {
    TorrentFile ParseTorrentFile(std::istream& stream);
    std::istream& operator>>(std::istream&, TorrentFile&);
}