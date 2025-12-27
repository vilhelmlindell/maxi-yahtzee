#include "run.hpp"
#include "mcts.hpp"
#include <iostream>
#include <thread>

#define GAME_COUNT 100
#define THREAD_COUNT 8
#define MILLISECONDS_PER_MOVE 1
// #define PRINT_INFO

uint64_t visits = 0;
uint64_t milliseconds = 0;

void run_games() {
    float average_score = 0;
    for (int i = 0; i < GAME_COUNT; i++) {
        Game game = Game(1);
        run_game(game);
        float score = game.players[0].total_score();
        average_score = average_score + (score - average_score) / (i + 1);
        Game::score_to_beat = (uint8_t)average_score;

#ifdef PRINT_INFO
        std::cout << "Winner: " << (int)game.winner() << " Score: " << score << std::endl;
#endif
        std::cout << "Average score: " << average_score << std::endl;
        std::cout << "Games played: " << i + 1 << std::endl << std::endl;
        uint64_t vps = (float)visits / ((float)milliseconds / 1000);
        std::cout << visits << " visits / " << milliseconds << " ms = " << vps << " vps" << std::endl;
    }
}

void run_game(Game& game) {
    while (!game.is_terminal()) {
        Move move = run_mcts(game);

#ifdef PRINT_INFO
        std::cout << "Player: " << (int)game.player_i << " " << move.to_string() << std::endl;
        if (move.type != Move::Type::Reroll) {
            std::cout << std::endl;
        }
#endif

        game.play_move(move);
#ifdef PRINT_INFO
        std::cout << game.scores_string() << std::endl;
#endif
    }
}

Move run_mcts(Game& game) {
    std::vector<std::unique_ptr<MCTSNode>> thread_roots;
    std::vector<std::thread> threads;
    thread_roots.reserve(THREAD_COUNT);
    threads.reserve(THREAD_COUNT);

    auto duration = std::chrono::milliseconds(MILLISECONDS_PER_MOVE);
    for (uint i = 0; i < THREAD_COUNT; i++) {
        thread_roots.push_back(std::make_unique<MCTSNode>(game));
        threads.push_back(std::thread([&, i]() {
            auto start = std::chrono::steady_clock::now();
            while (std::chrono::steady_clock::now() - start < duration) {
                thread_roots[i]->run_iteration();
            }
        }));
    }
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    MCTSNode* total = thread_roots.back().get();
    for (uint i = 0; i < THREAD_COUNT - 1; i++) {
        MCTSNode* root = thread_roots[i].get();
        total->visits += root->visits;
        total->wins += root->wins;
        for (auto& child : root->children) {
            for (auto& total_child : total->children) {
                if (child->move.value() == total_child->move.value()) {
                    total_child->visits += child->visits;
                    total_child->wins += child->wins;
                }
            }
        }
    }

#ifdef PRINT_INFO
    std::cout << total->children_string() << std::endl;
    // float seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    int vps = (float)total->visits / ((float)duration.count() / 1000.0);
    std::cout << total->visits << " visits / " << duration.count() << " ms = " << vps << " vps" << std::endl;
#endif

    visits += total->visits;
    milliseconds += duration.count();

    return total->children[0]->move.value();
}
