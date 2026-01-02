#ifndef MCTS_HPP
#define MCTS_HPP

#include "game.h"
#include "lookup.h"
#include <fstream>
#include <limits>
#include <memory>
#include <vector>

#define MAXIMIZE_AVERAGE_SCORE

struct MCTSNode {
    Game game;

    MCTSNode* parent = nullptr;
    std::vector<std::unique_ptr<MCTSNode>> children;
    std::optional<Move> move;
    uint32_t visits = 0;
    uint8_t player_i = 0;
    //std::vector<std::pair<uint8_t, float>> categories;

    DiceLookup* lookup;

    //double cached_ucb1 = 0;
    double cached_log_visits = 0;
    uint64_t cached_visits = 0;


#ifdef MAXIMIZE_AVERAGE_SCORE
    uint64_t total_score = 0;
#else
    uint32_t wins = 0;
#endif

    std::optional<uint8_t> category_i = 0;

    uint32_t score_mask = 0;
    uint32_t cross_mask = 0;
    uint8_t reroll_i = 0;

    MCTSNode(Game game);

    double compute_ucb1_reward();
    double ucb1();
    bool is_terminal();
    bool is_leaf_node();
    bool rerolls_left();
    bool categories_left();
    bool crosses_left();
    MCTSNode* select_child();
    MCTSNode* expand();
    uint8_t simulate();
    void backpropagate(Game& sim_game, uint8_t total_score);
    void run_iteration();
    Move next_move();
    MCTSNode* best_child() const;
    std::string node_string();
    std::string children_string();
};

#endif // MCTS_HPP
