#include "codes.h"
#include "infoboard.h"
#include "rows/empty_row.h"
#include "rows/download_progress_row.h"
#include "rows/connected_peers_row.h"
#include <sstream>
#include <memory>
#include <string>
#include <iostream>
#include <iomanip>

using namespace std::chrono;


InfoBoard::InfoBoard(int rowCount, milliseconds checkInterval) 
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