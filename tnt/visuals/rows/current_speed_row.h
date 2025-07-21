#pragma once

#include "../info_row.h"
#include "../../torrent_file/types.h"
#include <string>
#include <functional>
#include <tuple>
#include <thread>


class CurrentSpeedRow : public InfoRow {
public:
    CurrentSpeedRow(
        const TorrentFile& torrentFile,
        std::function<std::tuple<float>()> dataSrc
    );
    
    std::string GetValue() override;

private:
    std::function<std::tuple<float>()> dataSrc_;
    const TorrentFile& torrentFile_;
};
