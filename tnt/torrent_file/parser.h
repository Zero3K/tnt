#pragma once

#include "types.h"
#include <iostream>


TorrentFile ParseTorrentFile(std::istream& stream);
std::istream& operator>>(std::istream&, TorrentFile&);