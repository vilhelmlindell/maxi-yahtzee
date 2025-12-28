#include "mcts.h"
#include "game.h"
#include "utils.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

MCTSNode::MCTSNode(Game game) : game(game) {
    player_i = game.player_i;

    categories = &game.dice.categories().categories;
    score_mask = game.dice.categories().category_mask & game.player().scored_mask;
    cross_mask = game.player().scored_mask;
    fisher_i = categories->size();

    next_category = find_next_category();
}

double MCTSNode::uct() const {
    if (visits == 0)
        return INFINITY;
    if (parent == nullptr)
        return 0.0;

    const double C = 1.414;
    return (double)wins / (double)visits + C * sqrt(log(parent->visits) / visits);
}

bool MCTSNode::is_terminal() {
    return game.is_terminal();
}

bool MCTSNode::is_leaf_node() {
    return cross_mask || (reroll_rng_mask && game.player().rerolls > 0) || next_category.has_value() || is_terminal();
}

MCTSNode* MCTSNode::select_child() {
    MCTSNode* current = this;

    while (!current->is_leaf_node()) {
        auto selected = std::max_element(current->children.begin(), current->children.end(), [](const std::unique_ptr<MCTSNode>& a, const std::unique_ptr<MCTSNode>& b) {
            return a->uct() < b->uct();
        });
        current = selected->get();
    }

    return current;
}

MCTSNode* MCTSNode::expand() {
    if (is_terminal()) {
        return this;
    }

    Game new_game = game;
    Move move = next_move();
    new_game.play_move(move);

    std::unique_ptr<MCTSNode> child = std::make_unique<MCTSNode>(new_game);
    child->parent = this;
    child->move = move;
    child->player_i = player_i;

    children.push_back(std::move(child));

    return children.back().get();
}

uint8_t MCTSNode::simulate() {
    Game sim_game = game;
    return sim_game.playout();
}

void MCTSNode::backpropagate(uint8_t winner) {
    visits++;
    if (player_i == winner) {
        wins++;
    }

    if (parent != nullptr) {
        parent->backpropagate(winner);
    }
}

void MCTSNode::run_iteration() {
    MCTSNode* leaf = select_child();
    MCTSNode* node = leaf->expand();
    uint8_t winner = node->simulate();
    node->backpropagate(winner);
}

std::optional<CategoryEntry> MCTSNode::find_next_category() {
    while (fisher_i > 0) {
        fisher_i--;

        size_t i = fast_rand(fisher_i + 1);
        std::swap(categories->at(i), categories->at(fisher_i));

        CategoryEntry& entry = categories->at(fisher_i);
        uint32_t bit = 1u << (uint32_t)entry.category;

        if (score_mask & bit) {
            return entry;
        }
    }
    return std::nullopt;
}

Move MCTSNode::next_move() {
    Move move;

    if (next_category.has_value()) {
        move.type = Move::Type::Score;
        move.score_entry = next_category.value();

        next_category = find_next_category();
    } else if (reroll_rng_mask && game.player().rerolls > 0) {
        uint8_t i = random_set_bit_u64(reroll_rng_mask);

        move.type = Move::Type::Reroll;
        move.reroll_mask = i;

        reroll_rng_mask ^= 1 << i;
    } else if (cross_mask) {
        uint32_t i = random_set_bit_u32(cross_mask);
        if (i > 20) {
            std::cout << "Hmmmm: " << cross_mask << std::endl;
        }
        move.type = Move::Type::Cross;
        move.cross_i = i;
        cross_mask ^= 1 << i;
    } else {
        std::cerr << "panic: next_move found no move\n";
        std::terminate();
    }
    return move;
}

MCTSNode* MCTSNode::best_child() const {
    if (children.empty())
        return nullptr;

    auto best = std::max_element(children.begin(), children.end(), [](const std::unique_ptr<MCTSNode>& a, const std::unique_ptr<MCTSNode>& b) {
        return a->visits < b->visits;
    });

    return best->get();
}

void MCTSNode::write_dot(std::ofstream& out, int& node_id, int parent_id, int max_depth, int current_depth) {
    int my_id = node_id++;

    double win_rate = visits > 0 ? (double)wins / visits : 0.0;
    out << "  node" << my_id << " [label=\"";
    out << "V:" << visits << "\\n";
    out << "W:" << wins << "\\n";
    out << "WR:" << std::fixed << std::setprecision(2) << win_rate << "\"";

    if (visits > 10) {
        int r = (int)(255 * (1.0 - win_rate));
        int g = (int)(255 * win_rate);
        out << " style=filled fillcolor=\"#" << std::hex << std::setfill('0') << std::setw(2) << r << std::setw(2) << g << "00" << std::dec << "\"";
    }
    out << "];\n";

    if (parent_id >= 0) {
        out << "  node" << parent_id << " -> node" << my_id << ";\n";
    }

    if (current_depth < max_depth) {
        for (auto& child : children) {
            child->write_dot(out, node_id, my_id, max_depth, current_depth + 1);
        }
    }
}

void MCTSNode::save_tree(const char* filename, int max_depth) {
    std::ofstream out(filename);
    out << "digraph MCTS {\n";
    out << "  rankdir=TB;\n";
    out << "  node [shape=box];\n";
    int node_id = 0;
    write_dot(out, node_id, -1, max_depth);
    out << "}\n";
    out.close();
    printf("Tree saved to %s (use: xdot %s)\n", filename, filename);
}

std::string MCTSNode::node_string() {
    std::ostringstream oss;

    double winrate = visits > 0 ? (double)wins / (double)visits : 0.0;

    oss << "Visits: " << visits << " | Wins: " << wins << " | Winrate: " << std::fixed << std::setprecision(4) << winrate << " | UCT: " << std::setprecision(4) << uct() << " | ";

    if (move) {
        oss << "Move: " << move.value().to_string();
    } else {
        oss << "Move: None";
    }

    return oss.str();
}


std::string MCTSNode::children_string() {
    std::ostringstream oss;

    std::sort(children.begin(), children.end(), [](const std::unique_ptr<MCTSNode>& a, const std::unique_ptr<MCTSNode>& b) {
        return a->visits > b->visits;
    });

    for (int i = 5; i >= 0; --i) {
        oss << (int)game.dice.dice[i] << " ";
    }
    oss << std::endl;

    for (auto& node : children) {
        oss << node->node_string() << std::endl;
    }
    return oss.str();
}

