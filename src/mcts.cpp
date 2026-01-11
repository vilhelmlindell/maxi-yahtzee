#include "mcts.h"
#include "game.h"
#include "lookup.h"
#include "utils.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

thread_local int lowest_score = 1000;
thread_local int highest_score = 0;

MCTSNode::MCTSNode(Game game)
    : game(game) {
    player_i = game.player_i;

    lookup = &get_dice_lookup(game.dice);

    score_mask = lookup->category_mask & game.player().scored_mask;
    cross_mask = game.player().scored_mask;

    category_i = next_valid_category(*lookup, score_mask, category_i.value());
    rerolls = get_best_rerolls(game.dice, game.player().scored_mask);
}

double MCTSNode::compute_ucb1_reward() {
    // If we are purely trying to win
    // return (double)wins / (double)visits;
    // If we are trying to maximize average score
    double average_score = (double)total_score / (double)visits;
    //double normalized_reward = average_score / 500;
    if (highest_score < lowest_score) {
        return 0.5;
    }
    double normalized_reward = (average_score - lowest_score) / (highest_score - lowest_score);
    return normalized_reward;
}

double MCTSNode::ucb1() {
    if (visits == 0)
        return INFINITY;
    if (parent == nullptr)
        return 0.0;

    const double C = 1.414;
    return compute_ucb1_reward() + C * sqrt(parent->cached_log_visits / visits);
}

bool MCTSNode::is_terminal() {
    return game.is_terminal();
}

bool MCTSNode::is_leaf_node() {
    return categories_left() || rerolls_left() || crosses_left();
}

bool MCTSNode::rerolls_left() {
    return reroll_i < REROLLS && game.player().rerolls > 0;
}

bool MCTSNode::categories_left() {
    return category_i.has_value();
}

bool MCTSNode::crosses_left() {
    return cross_mask;
}

MCTSNode* MCTSNode::select_child() {
    MCTSNode* current = this;

    while (!current->is_leaf_node() && !current->is_terminal()) {
        auto selected = std::max_element(current->children.begin(), current->children.end(), [](const std::unique_ptr<MCTSNode>& a, const std::unique_ptr<MCTSNode>& b) {
            return a->ucb1() < b->ucb1();
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

void MCTSNode::backpropagate(Game& sim_game, int win_score) {
    visits++;
    total_score += win_score;
    // if (player_i == winner) {
    //     wins++;
    // }

    if (visits != 0) {
        cached_log_visits = std::log(visits);
    }

    if (parent != nullptr) {
        parent->backpropagate(sim_game, win_score);
    }
}

void MCTSNode::run_iteration() {
    MCTSNode* leaf = select_child();
    MCTSNode* node = leaf->expand();
    Game sim_game = node->game;
    sim_game.playout();
    int win_score = sim_game.players[0].total_score();
    lowest_score = std::min(lowest_score, win_score);
    highest_score = std::max(highest_score, win_score);
    node->backpropagate(sim_game, win_score);
}

Move MCTSNode::next_move() {
    Move move;

    if (categories_left()) {
        move.type = Move::Type::Score;
        move.score_entry = lookup->categories.at(category_i.value());
        uint32_t cat_mask = 1 << (int)move.score_entry.category;
        score_mask ^= cat_mask;
        cross_mask ^= cat_mask;
        category_i = next_valid_category(*lookup, score_mask, category_i.value());
    } else if (rerolls_left()) {
        move.type = Move::Type::Reroll;
        //move.reroll = lookup->rerolls[reroll_i];
        move.reroll = lookup->sorted_rerolls[rerolls[reroll_i]];
        reroll_i++;
    } else if (crosses_left()) {
        const uint32_t never_cross = ~0b10000000000000111100;
        uint32_t i;
        if (cross_mask & never_cross) {
            i = random_set_bit_u32(cross_mask & never_cross);
        } else {
            i = random_set_bit_u32(cross_mask);
        }
        move.type = Move::Type::Score;
        move.score_entry.category = (Category)i;
        move.score_entry.score = 0;
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

std::string MCTSNode::node_string() {
    std::ostringstream oss;

    double score = (double)total_score / (double)visits;

    oss << "Visits: " << visits << " | Average Score: " << std::fixed << std::setprecision(4) << score << " | UCT: " << std::setprecision(4) << ucb1() << " | ";

    if (move) {
        oss << "Move: " << move.value().to_string();
    } else {
        oss << "Move: None";
    }

    return oss.str();
}

std::string MCTSNode::children_string() {
    std::ostringstream oss;

    // non-owning view
    std::vector<MCTSNode*> view;
    view.reserve(children.size());

    for (auto& child : children) {
        view.push_back(child.get());
    }

    // sort the view, not the owners
    std::sort(view.begin(), view.end(),
        [](const MCTSNode* a, const MCTSNode* b) {
            return a->visits > b->visits;
        });

    for (MCTSNode* node : view) {
        oss << node->node_string() << '\n';
    }

    return oss.str();
}
