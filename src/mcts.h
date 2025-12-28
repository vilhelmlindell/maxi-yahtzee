#ifndef MCTS_HPP
#define MCTS_HPP

#include "game.h"
#include <fstream>
#include <memory>
#include <vector>

struct MCTSNode {
    Game game;
    std::vector<std::unique_ptr<MCTSNode>> children;
    MCTSNode* parent = nullptr;
    std::optional<Move> move;

    uint8_t player_i = 0;
    uint32_t visits = 0;
    uint32_t wins = 0;

    uint8_t fisher_i = 0;

    std::vector<CategoryEntry>* categories = nullptr;
    uint32_t score_mask = 0;
    uint32_t cross_mask = 0;
    std::optional<CategoryEntry> next_category;

    uint64_t reroll_rng_mask = 0xFFFFFFFFFFFFFFFF;

    MCTSNode(Game game);

    double uct() const;
    bool is_terminal();
    bool is_leaf_node();
    MCTSNode* select_child();
    MCTSNode* expand();
    uint8_t simulate();
    void backpropagate(uint8_t winner);
    void run_iteration();
    std::optional<CategoryEntry> find_next_category();
    Move next_move();
    MCTSNode* best_child() const;
    std::string node_string();
    std::string children_string();
    void write_dot(std::ofstream& out, int& node_id, int parent_id = -1, int max_depth = 3, int current_depth = 0);
    void save_tree(const char* filename, int max_depth = 3);
};

#endif // MCTS_HPP
