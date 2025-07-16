#include "visuals.h"
#include "codes.h"
#include <sstream>
#include <memory>
#include <string>
#include <iostream>
#include <iomanip>

using namespace std::chrono;


std::string EmptyRow::GetValue() {
    return "";
}


DownloadProgressBarRow::DownloadProgressBarRow(
    std::function<std::tuple<int, int>()> dataSrc
) : dataSrc_(dataSrc) {}

std::string DownloadProgressBarRow::GetValue() {
    auto [downloadedCnt, totalCnt] = dataSrc_();

    // yeah it looks pretty bad but i'll fix it
    
    std::stringstream sstr;
    sstr << CLEAR_LINE;

    int progress = downloadedCnt * 50 / totalCnt;

    sstr << GREEN << "Progress: [";
    
    for (int i = 0; i < progress; i++) 
        sstr << "=";
    if (progress != 50)
        sstr << ">";

    for (int i = 0; i < 50 - progress - 1; i++) 
        sstr << " ";
    sstr << "] ";

    float percentage = downloadedCnt * 100.0 / totalCnt;
    sstr << std::fixed << std::setprecision(2) << BOLD << percentage << "%";

    return sstr.str();
}


ConnectedPeersStatusRow::ConnectedPeersStatusRow(
    std::function<std::tuple<int, int>()> dataSrc
) : dataSrc_(dataSrc) {}

std::string ConnectedPeersStatusRow::GetValue() {
    auto [connectedCnt, totalCnt] = dataSrc_();

    static const std::vector<std::string> icons = {
        "[ / ]",
        "[ - ]",
        "[ \\ ]",
        "[ | ]"
    };
    static const std::string deadIcon = "[~_~]";

    static int i = 0;

    std::stringstream sstr;
    sstr << CLEAR_LINE << BOLD << LIGHT_GRAY;

    if (connectedCnt != 0)
        sstr << icons[i];
    else
        sstr << deadIcon;

    sstr << RESET << LIGHT_GRAY
        << " Peers connected: " << YELLOW << BOLD << connectedCnt
        << RESET << DARK_GRAY << " out of " << totalCnt << " found";

    i = (i + 1) % icons.size();

    return sstr.str();
}


InfoBoard::InfoBoard(int rowCount,  milliseconds checkInterval) 
    : checkInterval_(checkInterval), updateIntervals_(rowCount), 
    rows_(rowCount, std::make_shared<EmptyRow>()), lastDisplayed_(rowCount) {}

InfoBoard::~InfoBoard() {
    Stop();
}

void InfoBoard::Start() {
    MoveDown(rows_.size());

    running_ = true;
    thr_ = std::thread([&]() {
        while (running_) {
            time_point<steady_clock> tp = steady_clock::now();
            for (size_t i = 0; i < rows_.size(); i++) {
                if (tp - lastDisplayed_[i] > updateIntervals_[i])
                    DisplayRow(i);
            }

            std::this_thread::sleep_for(checkInterval_);
        }
    });
}

void InfoBoard::Stop() {
    running_ = false;
    if (thr_.joinable())
        thr_.join();
}

void InfoBoard::SetRow(int rowIdx, std::shared_ptr<InfoRow> row, 
        milliseconds updateInterval) {
    updateIntervals_[rowIdx] = updateInterval;
    rows_[rowIdx] = row;
}

void InfoBoard::DisplayRow(int rowIdx) {
    MoveUp(rows_.size() - rowIdx);
    std::cout << "\r" << RESET << rows_[rowIdx]->GetValue();
    lastDisplayed_[rowIdx] = steady_clock::now();
    MoveDown(rows_.size() - rowIdx);
}

void InfoBoard::MoveUp(int cnt) {
    for (int i = 0; i < cnt; i++)
        std::cout << MOVE_UP;
}

void InfoBoard::MoveDown(int cnt) {
    std::cout << std::string(cnt, '\n');
}