#include "run.h"
#include "mcts.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

extern thread_local uint32_t rng_state;

int64_t visits = 0;
int64_t milliseconds = 0;
float average_score = 0;

static std::vector<std::thread> pool;
static std::queue<std::function<void()>> tasks;
static std::mutex pool_mutex;
static std::condition_variable pool_cv;
static bool pool_stop = false;

static void ensure_pool_exists(int num_threads) {
    if (!pool.empty())
        return;
    for (int i = 0; i < num_threads; i++) {
        pool.emplace_back([] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(pool_mutex);
                    pool_cv.wait(lock, [] {
                        return pool_stop || !tasks.empty();
                    });
                    if (pool_stop && tasks.empty())
                        return;
                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task();
            }
        });
    }
}

void print_usage(char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]\n"
              << "Options:\n"
              << "  -g <int>    Number of games (required)\n"
              << "  -m <int>    Milliseconds per move (required)\n"
              << "  -t <int>    Number of threads (default: 1)\n"
              << "  -d          Enable debug mode\n";
}

void run_args(int argc, char* argv[]) {
    Config config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-g" && i + 1 < argc) {
            config.games = std::stoi(argv[++i]);
        } else if (arg == "-t" && i + 1 < argc) {
            config.ms_per_move = std::stoi(argv[++i]);
        } else if (arg == "-m" && i + 1 < argc) {
            config.threads = std::stoi(argv[++i]);
        } else if (arg == "-d") {
            config.debug = true;
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << std::endl;
            print_usage(argv[0]);
            return;
        }
    }

    if (config.games <= 0 || config.ms_per_move <= 0) {
        std::cerr << "Error: Games and ms_per_move are required and must be positive.\n";
        print_usage(argv[0]);
        return;
    }

    std::cout << "Running " << config.games << " games on " << config.threads << " threads with " << config.ms_per_move << "ms per move..." << std::endl;
    run_games(config);
}

void run_games(Config config) {
    for (int i = 0; i < config.games; i++) {
        Game game = Game(1);
        game.dice = Dice({1, 1, 4, 0, 0, 0});
        run_game(game, config);
        int score = game.players[0].total_score();
        average_score = average_score + ((float)score - average_score) / (i + 1);

        std::cout << "Average score: " << average_score << std::endl;
        std::cout << "Games played: " << i + 1 << std::endl << std::endl;
    }
}

void run_game(Game& game, Config config) {
    while (!game.is_terminal()) {
        Move move = run_mcts(game, config);
        game.play_move(move);
    }
    int score = game.players[0].total_score();
    int64_t vps = (float)visits / ((float)milliseconds / 1000);
    std::cout << game.scores_string() << std::endl;
    std::cout << score << " " << vps << std::endl;
    std::cout << visits << " visits / " << milliseconds << " ms = " << vps << " vps" << std::endl;
}

Move run_mcts(Game& game, Config config) {
    ensure_pool_exists(config.threads);

    std::vector<std::unique_ptr<MCTSNode>> thread_roots;
    std::atomic<int> completed_tasks{0};
    auto duration = std::chrono::milliseconds(config.ms_per_move);

    for (int i = 0; i < config.threads; i++) {
        auto node_ptr = std::make_unique<MCTSNode>(game);
        MCTSNode* raw_node_ptr = node_ptr.get();
        thread_roots.push_back(std::move(node_ptr));

        {
            std::lock_guard<std::mutex> lock(pool_mutex);
            tasks.push([raw_node_ptr, duration, &completed_tasks]() {
                rng_state = std::hash<std::thread::id>{}(std::this_thread::get_id());

                auto start = std::chrono::steady_clock::now();
                while (std::chrono::steady_clock::now() - start < duration) {
                    raw_node_ptr->run_iteration();
                }
                completed_tasks++;
            });
        }
        pool_cv.notify_one();
    }

    while (completed_tasks < config.threads) {
        std::this_thread::yield();
    }

    MCTSNode* total = thread_roots.back().get();
    for (int i = 0; i < config.threads - 1; i++) {
        MCTSNode* root = thread_roots[i].get();
        total->visits += root->visits;
        total->total_score += root->total_score;

        for (auto& child : root->children) {
            for (auto& total_child : total->children) {
                if (child->move.value() == total_child->move.value()) {
                    total_child->visits += child->visits;
                    total_child->total_score += child->total_score;
                }
            }
        }
    }

    visits += total->visits;
    milliseconds += duration.count();
    if (config.debug) {
        std::cout << game.dice.to_string() << std::endl;
        std::cout << total->children_string() << std::endl;
        std::cout << total->children[0]->move.value().to_string() << std::endl;
    }
    return total->children[0]->move.value();
}
