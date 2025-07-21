#pragma once

#include "../info_row.h"
#include <string>
#include <functional>
#include <tuple>
#include <thread>


class ConnectedPeersStatusRow : public InfoRow {
public:
    ConnectedPeersStatusRow(
        std::function<std::tuple<int, int>()> dataSrc
    );
    
    std::string GetValue() override;

private:
    std::function<std::tuple<int, int>()> dataSrc_;
};
