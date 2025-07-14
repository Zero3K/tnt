#include "piece_storage/piece_storage.h"
#include "torrent_file/parser.h"
#include "torrent_file/types.h"
#include "torrent_tracker.h"
#include "peer_connection/peer_connection.h"
#include "codes.h"
#include "conductor/conductor.h"
#include <cxxopts.hpp>
#include <exception>
#include <fstream>
#include <iostream>
#include <ostream>
#include <thread>
#include <chrono>
#include <random>

using namespace std::chrono_literals;
namespace fs = std::filesystem;

std::mutex coutMtx_;

void PrintProgressBar(int downloadedCnt, int totalCnt, float speed) {
    coutMtx_.lock();

    std::cout << MOVE_UP << "\r" << RESET << CLEAR_LINE;

    int progress = downloadedCnt * 50 / totalCnt;

    std::cout << GREEN << BOLD << "Downloading... [" << RESET << GREEN;
    
    for (int i = 0; i < progress; i++) 
        std::cout << "=";
    if (progress != 50)
        std::cout << ">";

    for (int i = 0; i < 50 - progress - 1; i++) 
        std::cout << " ";
    std::cout << "] " << downloadedCnt << "/" << totalCnt << " ";
    std::cout << speed << " MB/s\n";

    coutMtx_.unlock();
}

void PrintPeersCount(int connectedCnt, int totalCnt) {
    static std::string icons = "/-\\|";
    static int i = 0;

    coutMtx_.lock();

    std::cout << MOVE_UP << MOVE_UP << "\r" << RESET << CLEAR_LINE;
    std::cout << BOLD << LIGHT_GRAY << "[";

    if (connectedCnt != 0)
        std::cout << icons[i];
    else
        std::cout << "x";

    std::cout << "]" << RESET << LIGHT_GRAY
        << " Peers connected: " << YELLOW << BOLD << connectedCnt
        << RESET << DARK_GRAY << " out of " << totalCnt << " found\n\n";

    coutMtx_.unlock();

    i = (i + 1) % icons.size();
}

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);

    std::cout.precision(1);
    std::cout.setf(std::ios::fixed);

    // =========================================================

    cxxopts::Options options("tnt", "tnt is a simple torrent client");
    options.add_options()
        ("h,help", "Print usage")
        ("o,out", "Output file name.", cxxopts::value<std::string>())
        ("file", "Metainfo (.torrent) file.", cxxopts::value<std::string>());

    options.parse_positional({"file"});
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        options.positional_help("<torrent file>");
        std::cout << options.help() << std::endl;
        return 0;
    }

    // =========================================================

    fs::path tfPath = result["file"].as<std::string>();
    fs::path outputFilePath = result["out"].as<std::string>();
    
    TorrentFile file;
    std::ifstream stream(tfPath);
    stream >> file;

    std::cout << " - name: " << file.name << std::endl;
    std::cout << " - announce: " << file.announce << std::endl;
    std::cout << " - description: " << file.comment << std::endl;
    // std::cout << " - piece size: " << file.pieceLength << std::endl;
    // std::cout << " - pieces: " << file.pieceHashes.size() << std::endl << std::endl;

    TorrentTracker tracker(file.announce);
    tracker.UpdatePeers(file, "TTST0APP1DONT2WORRY3", 12345);
    auto peers = tracker.GetPeers();

    std::ofstream outputFile(outputFilePath);
    PieceStorage pieceStorage(file, outputFile);

    auto rng = std::default_random_engine();
    std::shuffle(peers.begin(), peers.end(), rng);

    Conductor cond(peers, file, pieceStorage);
    std::vector<std::thread> threads;

    threads.emplace_back([ptr = &cond] { ptr->Download(); });

    // =========================================================


    std::chrono::time_point tpStart = std::chrono::steady_clock::now();

    std::atomic<int> connCnt = 0;
    auto downloadProgressBarJob = [&]() {
        while (true) {
            int downloadedCnt = pieceStorage.GetFinishedCount();

            std::chrono::time_point tpCur = std::chrono::steady_clock::now();

            // // TODO: rework
            float avgSpeed = 1000.0 * downloadedCnt * file.pieceLength /
                std::chrono::duration_cast<std::chrono::milliseconds>(tpCur - tpStart).count() / 1024 / 1024;
            // std::cout << avgSpeed << " MB/s\n";

            PrintProgressBar(downloadedCnt, file.pieceHashes.size(), avgSpeed);
            if (downloadedCnt == file.pieceHashes.size())
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    };

    auto peersConnectedCntJob = [&]() {
        do {
            PrintPeersCount(cond.GetConnectedCount(), peers.size());
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        } while (pieceStorage.GetFinishedCount() < pieceStorage.GetTotalCount() || connCnt > 0);
        PrintPeersCount(cond.GetConnectedCount(), peers.size());
    };
    
    std::cout << "\n\n\n";
    threads.emplace_back(downloadProgressBarJob);
    threads.emplace_back(peersConnectedCntJob);

    while (pieceStorage.GetFinishedCount() < pieceStorage.GetTotalCount())
        std::this_thread::sleep_for(1000ms);

    for (auto& t : threads)
        t.join();

    return 0;
}