#include "download_progress_row.h"
#include "../codes.h"
#include <iomanip>
#include <sstream>


DownloadProgressBarRow::DownloadProgressBarRow(
    std::function<std::tuple<int, int, bool>()> dataSrc
) : dataSrc_(dataSrc) {}

std::string DownloadProgressBarRow::GetValue() {
    auto [downloadedCnt, totalCnt, isEndgame] = dataSrc_();

    // yeah it looks pretty bad but i'll fix it
    
    std::stringstream sstr;
    sstr << CLEAR_LINE;

    int progress = downloadedCnt * 20 / totalCnt;

    sstr << LIGHT_GRAY << " ~ " << "Progress: " << GREEN << "[";
    
    for (int i = 0; i < progress; i++) 
        sstr << "=";
    if (progress != 20)
        sstr << ">";


    sstr << VERY_DARK_GRAY;
    for (int i = 0; i < 20 - progress - 1; i++) 
        sstr << "=";
    sstr << GREEN << "] ";

    float percentage = downloadedCnt * 100.0 / totalCnt;
    sstr << std::fixed << std::setprecision(2) << BOLD << percentage << "%";
    if (isEndgame) {
        sstr << YELLOW << " ↯";
    }

    return sstr.str();
}