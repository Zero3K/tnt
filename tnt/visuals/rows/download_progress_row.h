#pragma once

#include "../info_row.h"
#include <string>
#include <functional>
#include <tuple>
#include <thread>


class DownloadProgressBarRow : public InfoRow {
public:
    DownloadProgressBarRow(
        std::function<std::tuple<int, int, bool>()> dataSrc
    );
    
    std::string GetValue() override;

private:
    std::function<std::tuple<int, int, bool>()> dataSrc_;
};