#include "piece_storage/piece_storage.h"
#include "torrent_file/parser.h"
#include "torrent_file/types.h"
#include "torrent_tracker.h"
#include "peer_connection/peer_connection.h"
#include "visuals/visuals.h"
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
 
    InfoBoard board(4, 100ms);

    auto tp = std::chrono::steady_clock::now();
    auto progressRow = std::make_shared<DownloadProgressBarRow>([&]{
        return std::tuple<int, int>{
            pieceStorage.GetFinishedCount(),
            file.pieceHashes.size()
        };
    });
    board.SetRow(3, progressRow, 200ms);

    auto peersRow = std::make_shared<ConnectedPeersStatusRow>([&]{
        return std::tuple<int, int>{
            cond.GetConnectedCount(),
            peers.size()
        };
    });
    board.SetRow(2, peersRow, 300ms);

    board.Start();

    while (pieceStorage.GetFinishedCount() < pieceStorage.GetTotalCount())
        std::this_thread::sleep_for(1000ms);
    std::this_thread::sleep_for(1000ms);
    
    board.Stop();

    for (auto& t : threads)
        t.join();

    return 0;
}