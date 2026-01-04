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

thread_local int lowest_score = 0;
thread_local int highest_score = 0;

MCTSNode::MCTSNode(Game game)
    : game(game) {
    player_i = game.player_i;

    lookup = &get_dice_lookup(game.dice);

    if (lookup == nullptr) {
        std::cout << "Oh no" << game.dice.to_string() << std::endl;
    }
    score_mask = lookup->category_mask & game.player().scored_mask;
    cross_mask = game.player().scored_mask;

    category_i = next_valid_category(*lookup, score_mask, category_i.value());

    //for (size_t i = 0; i < lookup->categories.size(); i++) {
    //    CategoryEntry entry = lookup->categories.at(i);
    //    if (score_mask & 1 << (int)entry.category) {

    //        //bool is_upper = (int)entry.category >= (int)Category::Threes && (int)entry.category <= (int)Category::Sixes;
    //        //if (is_upper) {
    //        //    if ()
    //        //}
    //        float heuristic = get_score_heuristic(entry);
    //        if (heuristic == 0.0) {
    //            continue;
    //        }
    //        std::pair<uint8_t, float> new_pair = {i, heuristic};

    //        // Find the insertion point (sorting by heuristic descending)
    //        auto it = categories.begin();
    //        while (it != categories.end() && it->second > heuristic) {
    //            ++it;
    //        }

    //        // insert() handles the shifting of elements for you
    //        categories.insert(it, new_pair);
    //    }
    //}
}

double MCTSNode::compute_ucb1_reward() {
    // If we are purely trying to win
    // return (double)wins / (double)visits;
    // If we are trying to maximize average score
    double average_score = (double)total_score / (double)visits;
    double normalized_reward = (average_score / 300);
    // double normalized_reward = (average_score - lowest_score) / (highest_score - lowest_score);
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
    const int REROLLS = 1;
    return reroll_i < REROLLS && game.player().rerolls > 0;
}

bool MCTSNode::categories_left() {
    return category_i.has_value();
}

bool MCTSNode::crosses_left() {
    return cross_mask;
}

// MCTSNode* MCTSNode::select_child() {
//     MCTSNode* current = this;
//
//     // A node is only a "leaf" for selection purposes if it hasn't
//     // exhausted its moves OR if it's terminal.
//     // If it's FULLY expanded, we MUST descend.
//     while (!current->is_terminal()) {
//         if (current->is_leaf_node()) {
//             // This node still has moves to expand, so stop here and expand one.
//             return current;
//         }
//
//         // If we get here, the node is fully expanded.
//         // Use UCB1 to pick which child to descend into.
//         auto selected = std::max_element(current->children.begin(), current->children.end(),
//             [](const std::unique_ptr<MCTSNode>& a, const std::unique_ptr<MCTSNode>& b) {
//                 return a->ucb1() < b->ucb1();
//             });
//         current = selected->get();
//     }
//     return current;
// }
MCTSNode* MCTSNode::select_child() {
    MCTSNode* current = this;

    if (current == nullptr) {
        std::cerr << "1 Error current nullptr" << std::endl;
        std::terminate();
    }

    while (!current->is_leaf_node() && !current->is_terminal()) {
        if (visits != cached_visits) {
            cached_log_visits = log((double)visits);
            cached_visits = visits;
        }
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

void MCTSNode::backpropagate(Game& sim_game, uint8_t win_score) {
    visits++;
    total_score += win_score;
    // if (player_i == winner) {
    //     wins++;
    // }

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
        score_mask ^= 1 << (int)move.score_entry.category;
        category_i = next_valid_category(*lookup, score_mask, category_i.value());
    } else if (rerolls_left()) {
        move.type = Move::Type::Reroll;
        //move.reroll = lookup->rerolls[reroll_i];
        move.reroll = get_best_reroll(game.dice, game.player().scored_mask);
        reroll_i++;
    } else if (crosses_left()) {
        const uint32_t never_cross = ~0b10000000000000111100;
        uint32_t i;
        if (cross_mask & never_cross) {
            i = random_set_bit_u32(cross_mask & never_cross);
        } else {
            i = random_set_bit_u32(cross_mask);
        }
        move.type = Move::Type::Cross;
        move.crossed_category = (Category)i;
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

    std::sort(children.begin(), children.end(), [](const std::unique_ptr<MCTSNode>& a, const std::unique_ptr<MCTSNode>& b) {
        return a->visits > b->visits;
    });

    for (auto& node : children) {
        oss << node->node_string() << std::endl;
    }
    return oss.str();
}
