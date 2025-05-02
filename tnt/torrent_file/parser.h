#pragma once

#include "types.h"
#include <iostream>


namespace TorrentFile {
    Metainfo ParseTorrentFile(std::istream& stream);
    std::istream& operator>>(std::istream&, Metainfo&);
}