#include "piece_storage/piece_storage.h"
#include "torrent_file/parser.h"
#include "torrent_file/types.h"
#include "torrent_tracker.h"
#include "peer_connection/peer_connection.h"
#include "visuals/infoboard.h"
#include "visuals/rows/download_progress_row.h"
#include "visuals/rows/connected_peers_row.h"
#include "visuals/rows/current_speed_row.h"
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

int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);

    // =========================================================

    cxxopts::Options options("tnt", "tnt is a simple torrent client");
    options.add_options()
        ("h,help", "Print usage")
        ("q,quiet", "Minimal output")
        ("o,out", "Output file name", cxxopts::value<std::string>())
        ("file", "Metainfo (.torrent) file", cxxopts::value<std::string>());

    options.parse_positional({"file"});
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        options.positional_help("torrent_file");
        std::cout << options.help() << std::endl;
        return 0;
    }

    bool quiet = result.count("quiet");

    // =========================================================

    fs::path tfPath = result["file"].as<std::string>();
    fs::path outputFilePath = result["out"].as<std::string>();
    
    TorrentFile file;
    std::ifstream stream(tfPath);
    stream >> file;

    if (!quiet) {
        std::cout << " > Announce: " <<  file.announce << std::endl;
        if (file.comment.has_value())
            std::cout << " > Comment: " << file.comment.value() << std::endl;
    }

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
 
    InfoBoard board(4, 100ms);

    auto tp = std::chrono::steady_clock::now();
    auto progressRow = std::make_shared<DownloadProgressBarRow>([&]{
        return std::tuple<int, int, bool>{
            pieceStorage.GetFinishedCount(),
            file.info.pieces.size(),
            cond.isEndgame()
        };
    });
    board.SetRow(3, progressRow, 200ms);

    auto peersRow = std::make_shared<ConnectedPeersStatusRow>([&]{
        return std::tuple<int, int>{
            cond.GetConnectedCount(),
            peers.size()
        };
    });
    board.SetRow(1, peersRow, 300ms);

    auto speedRow = std::make_shared<CurrentSpeedRow>(file, [&]{
        return std::tuple<float>{
            static_cast<float>(cond.GetSpeed())
        };
    });
    board.SetRow(2, speedRow, 1s);

    if (!quiet)
        board.Start();

    while (pieceStorage.GetFinishedCount() < pieceStorage.GetTotalCount())
        std::this_thread::sleep_for(1s);

    for (auto& t : threads)
        t.join();
    
    std::this_thread::sleep_for(1s);
    board.Stop();

    if (quiet) {
        std::cout << "download finished"  << std::endl;
    } else {
        std::cout << std::endl << " > Download finished" << std::endl;
    }

    return 0;
}