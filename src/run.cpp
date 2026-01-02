#include "run.h"
#include "mcts.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <thread>

extern thread_local uint32_t rng_state;

//#define PRINT_INFO

int64_t visits = 0;
int64_t milliseconds = 0;
float average_score = 0;

// Thread Pool State
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

void run_interface() {
    int games;
    int ms_per_move;
    int num_threads;

    while (true) {
        std::cout << "Enter: <games> <ms_per_move> <threads> (Ctrl+D to quit): ";

        if (!(std::cin >> games >> ms_per_move >> num_threads)) {
            std::cout << "\nInput closed, exiting." << std::endl;
            break; // EOF or invalid input
        }

        if (games <= 0) {
            std::cerr << "Number of games must be positive." << std::endl;
            continue;
        }

        if (ms_per_move <= 0) {
            std::cerr << "Milliseconds per move must be positive." << std::endl;
            continue;
        }

        if (num_threads <= 0) {
            std::cerr << "Thread count must be positive." << std::endl;
            continue;
        }

        run_games(games, ms_per_move, num_threads);
    }
}

void run_games(int game_count, int ms_per_move, int num_threads) {
    for (int i = 0; i < game_count; i++) {
        Game game = Game(1);
        game.dice = Dice({1, 1, 4, 0, 0, 0});
        run_game(game, ms_per_move, num_threads);
        int score = game.players[0].total_score();
        average_score = average_score + ((float)score - average_score) / (i + 1);

        std::cout << "Average score: " << average_score << std::endl;
        std::cout << "Games played: " << i + 1 << std::endl << std::endl;
    }
}

void run_game(Game& game, int ms_per_move, int num_threads) {
    while (!game.is_terminal()) {
        Move move = run_mcts(game, ms_per_move, num_threads);
        game.play_move(move);
    }
    int score = game.players[0].total_score();
    int64_t vps = (float)visits / ((float)milliseconds / 1000);
    std::cout << game.scores_string() << std::endl;
    std::cout << score << " " << vps << std::endl;
    std::cout << visits << " visits / " << milliseconds << " ms = " << vps << " vps" << std::endl;
}

Move run_mcts(Game& game, int ms_per_move, int num_threads) {
    ensure_pool_exists(num_threads);

    std::vector<std::unique_ptr<MCTSNode>> thread_roots;
    std::atomic<int> completed_tasks{0};
    auto duration = std::chrono::milliseconds(ms_per_move);

    // 1. Dispatch tasks
    for (int i = 0; i < num_threads; i++) {
        // 1. Create the node
        auto node_ptr = std::make_unique<MCTSNode>(game);
        // 2. Get the raw pointer to pass to the thread
        MCTSNode* raw_node_ptr = node_ptr.get();
        // 3. Move ownership into the vector
        thread_roots.push_back(std::move(node_ptr));

        {
            std::lock_guard<std::mutex> lock(pool_mutex);
            // Capture raw_node_ptr by VALUE, not thread_roots by reference
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

    // 2. Wait for this move's batch to finish
    while (completed_tasks < num_threads) {
        std::this_thread::yield();
    }

    // 3. Merge results (Reduction)
    MCTSNode* total = thread_roots.back().get();
    for (int i = 0; i < num_threads - 1; i++) {
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
#ifdef PRINT_INFO
    std::cout << game.dice.to_string() << std::endl;
    std::cout << total->children_string() << std::endl;
    std::cout << total->children[0]->move.value().to_string() << std::endl;
#endif
    return total->children[0]->move.value();
}
