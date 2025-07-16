#pragma once

#include <string>
#include <functional>
#include <tuple>
#include <thread>

class InfoRow {
public:
    /*
     * Get string to display.
     */
    virtual std::string GetValue() = 0;
};


class EmptyRow : public InfoRow {
public:
    std::string GetValue() override;
};


class DownloadProgressBarRow : public InfoRow {
public:
    DownloadProgressBarRow(
        std::function<std::tuple<int, int, bool>()> dataSrc
    );
    
    std::string GetValue() override;

private:
    std::function<std::tuple<int, int, bool>()> dataSrc_;
};


class ConnectedPeersStatusRow : public InfoRow {
public:
    ConnectedPeersStatusRow(
        std::function<std::tuple<int, int>()> dataSrc
    );
    
    std::string GetValue() override;

private:
    std::function<std::tuple<int, int>()> dataSrc_;
};



class InfoBoard {
public:
    InfoBoard(int rowCount, std::chrono::milliseconds checkInterval);
    ~InfoBoard();

    /*
     * Starts the infoboard updater thread.
     */
    void Start();

    /*
     * Stops the infoboard updater thread.
     */
    void Stop();

    void SetRow(int rowIdx, std::shared_ptr<InfoRow> row, std::chrono::milliseconds updateInterval);
private:
    void DisplayRow(int rowIdx);
    void MoveUp(int cnt);
    void MoveDown(int cnt);

    std::thread thr_;
    std::atomic<int> running_;

    std::chrono::milliseconds checkInterval_;
    std::vector<std::chrono::milliseconds> updateIntervals_;
    std::vector<std::shared_ptr<InfoRow>> rows_;
    std::vector<std::chrono::time_point<std::chrono::steady_clock>> lastDisplayed_;
};