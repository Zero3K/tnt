#include "current_speed_row.h"
#include "../codes.h"
#include <iomanip>
#include <sstream>


CurrentSpeedRow::CurrentSpeedRow(
    const TorrentFile& torrentFile,
    std::function<std::tuple<float>()> dataSrc
) : dataSrc_(dataSrc), torrentFile_(torrentFile) {}

std::string CurrentSpeedRow::GetValue() {
    auto [speed] = dataSrc_();

    std::stringstream sstr;
    sstr << CLEAR_LINE << RESET << LIGHT_GRAY << " ~ "
        << "Current speed: " << YELLOW << BOLD 
        << std::fixed << std::setprecision(2) 
        << speed * torrentFile_.pieceLength / 1e6 << " MB/s" << RESET;

    return sstr.str();
}
