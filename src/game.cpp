#include "game.h"
#include "lookup.h"
#include "utils.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

const char* category_to_string(Category c) {
    switch (c) {
    case Category::Ones:
        return "Ones";
    case Category::Twos:
        return "Twos";
    case Category::Threes:
        return "Threes";
    case Category::Fours:
        return "Fours";
    case Category::Fives:
        return "Fives";
    case Category::Sixes:
        return "Sixes";
    case Category::Pair:
        return "Pair";
    case Category::TwoPair:
        return "TwoPair";
    case Category::ThreePair:
        return "ThreePair";
    case Category::ThreeKind:
        return "ThreeKind";
    case Category::FourKind:
        return "FourKind";
    case Category::FiveKind:
        return "FiveKind";
    case Category::SmallStraight:
        return "SmallStraight";
    case Category::LargeStraight:
        return "LargeStraight";
    case Category::Straight:
        return "Straight";
    case Category::House:
        return "House";
    case Category::Villa:
        return "Villa";
    case Category::Tower:
        return "Tower";
    case Category::Chance:
        return "Chance";
    case Category::MaxiYahtzee:
        return "MaxiYahtzee";
    case Category::Count:
        return "Count";
    default:
        return "Unknown";
    }
}

Category category_from_string(std::string_view s) {
    if (s == "Ones")
        return Category::Ones;
    if (s == "Twos")
        return Category::Twos;
    if (s == "Threes")
        return Category::Threes;
    if (s == "Fours")
        return Category::Fours;
    if (s == "Fives")
        return Category::Fives;
    if (s == "Sixes")
        return Category::Sixes;
    if (s == "Pair")
        return Category::Pair;
    if (s == "TwoPair")
        return Category::TwoPair;
    if (s == "ThreePair")
        return Category::ThreePair;
    if (s == "ThreeKind")
        return Category::ThreeKind;
    if (s == "FourKind")
        return Category::FourKind;
    if (s == "FiveKind")
        return Category::FiveKind;
    if (s == "SmallStraight")
        return Category::SmallStraight;
    if (s == "LargeStraight")
        return Category::LargeStraight;
    if (s == "Straight")
        return Category::Straight;
    if (s == "House")
        return Category::House;
    if (s == "Villa")
        return Category::Villa;
    if (s == "Tower")
        return Category::Tower;
    if (s == "Chance")
        return Category::Chance;
    if (s == "MaxiYahtzee")
        return Category::MaxiYahtzee;
    if (s == "Count")
        return Category::Count;

    return Category::Count;
}

bool is_bonus_category(Category c) {
    return (int)c >= (int)Category::Threes && (int)c <= (int)Category::Sixes;
}

int Player::total_score() {
    int sum = 0;
    int i = 0;
    for (; i < 6; i++) {
        sum += *scores[i];
    }
    if (sum >= 75) {
        sum += 50;
    }
    for (; i < (int)Category::Count; i++) {
        sum += *scores[i];
    }
    return sum;
}

std::string Reroll::to_string() {
    std::ostringstream oss;
    for (int i = 0; i < 6; i++) {
        oss << (int)hold_freq[i] << " ";
    }
    return oss.str();
}

Dice::Dice(std::array<uint8_t, 6> dice_freq) {
    this->dice_freq = dice_freq;
}

Dice::Dice() {
    reroll_all();
}

void Dice::reroll_all() {
    dice_freq.fill(0);
    for (int i = 0; i < 6; i++) {
        dice_freq[fast_rand(6)]++;
    }
}

void Dice::reroll(Reroll const& reroll) {
    dice_freq = reroll.hold_freq;
    int num_rolls = reroll.num_rolls;
    while (num_rolls > 0) {
        dice_freq[fast_rand(6)]++;
        num_rolls--;
    }
}

std::string Dice::to_string() {
    std::ostringstream oss;

    for (int i = 0; i < 6; i++) {
        oss << (int)dice_freq[i] << " ";
    }
    return oss.str();
}

Move::Move() {}
//    , cross_i(Category::Twos) {}

std::string Move::to_string() {
    std::ostringstream oss;

    switch (type) {
    case Move::Type::Reroll: {
        oss << "reroll " << reroll.to_string();
        break;
    }
    case Move::Type::Cross: {
        oss << "cross " << category_to_string(crossed_category);
        break;
    }
    case Move::Type::Score: {
        oss << "score " << category_to_string(score_entry.category) << " " << (int)score_entry.score;
        // snprintf(buffer, sizeof(buffer), "score %s %d", category_to_string(score_entry.category), score_entry.score);
        break;
    }
    }

    return oss.str();
}

Move Move::from_string(const std::string& input) {
    std::istringstream iss(input);
    std::string cmd, arg1, arg2;

    iss >> cmd >> arg1;
    if (cmd.empty() || arg1.empty())
        throw std::invalid_argument("Invalid move format");

    Move m;

    if (cmd == "reroll") {
        uint32_t mask = parse_uint(arg1);
        if (mask > 0xFF)
            throw std::out_of_range("Reroll mask must fit in uint8_t");

        m.type = Move::Type::Reroll;
        // TODO: Finish
        // m.reroll_mask = static_cast<uint8_t>(mask);
    } else if (cmd == "cross") {
        uint32_t index = parse_uint(arg1);
        if (index > 0xFF)
            throw std::out_of_range("Cross index must fit in uint8_t");

        m.type = Move::Type::Cross;
        m.crossed_category = (Category)index;
    } else if (cmd == "score") {
        iss >> arg2;
        m.type = Move::Type::Score;
        m.score_entry.category = category_from_string(arg1);
        m.score_entry.score = parse_uint(arg2);
    } else {
        throw std::invalid_argument("Unknown move type: " + cmd);
    }

    return m;
}

Game::Game(int num_players) {
    for (int i = 0; i < num_players; i++) {
        players.push_back(Player());
    }
}

bool Game::is_terminal() {
    return rounds == (int)(Category::Count);
}

uint8_t Game::winner() {
    std::vector<uint8_t> scores = std::vector<uint8_t>(players.size());
    for (size_t i = 0; i < players.size(); i++) {
        scores[i] = players[i].total_score();
    }
    if (players.size() == 1) {
        return score_to_beat > scores[0];
    } else {
        auto it = std::max_element(scores.begin(), scores.end());
        return std::distance(scores.begin(), it);
    }
}

Player& Game::player() {
    return players[player_i];
}

void Game::next_player() {
    player_i++;
    if (player_i == players.size()) {
        player_i = 0;
        rounds++;
    }
    dice.reroll_all();
    player().rerolls += 2;
}

void Game::play_move(Move move) {
    switch (move.type) {
    case Move::Type::Score: {
        CategoryEntry entry = move.score_entry;
        size_t i = (size_t)entry.category;
        player().scores[i] = entry.score;
        player().scored_mask ^= 1 << i;
        if ((int)entry.category >= (int)Category::Ones && (int)entry.category <= (int)Category::Sixes) {
            player().bonus_progress += entry.score;
        }
        next_player();
        break;
    }
    case Move::Type::Reroll: {
        Reroll reroll = move.reroll;
        dice.reroll(reroll);
        player().rerolls--;
        break;
    }
    case Move::Type::Cross: {
        int i = (int)move.crossed_category;
        player().scores[i] = 0;
        player().scored_mask ^= 1 << i;
        next_player();
        break;
    }
    }
}

void Game::playout() {
    while (!is_terminal()) {
        DiceLookup& lookup = get_dice_lookup(dice);
        // std::cout << dice.to_string() << std::endl;
        // Move move;
        uint32_t combined = lookup.category_mask & player().scored_mask;

        if (lookup.categories.size() >= 1 && fast_rand(2)) {
            int start = fast_rand(1 + lookup.categories.size() / 3);
            // int start = 0;

            std::optional<uint8_t> i = next_valid_category(lookup, combined, start);
            if (!i.has_value()) {
                break;
            }

            CategoryEntry category = lookup.categories[i.value()];

            player().scores[(int)category.category] = category.score;
            player().scored_mask ^= (int)category.category;
            if ((int)category.category >= (int)Category::Ones && (int)category.category <= (int)Category::Sixes) {
                player().bonus_progress += category.score;
            }
            next_player();
            continue;
        }

        if (player().rerolls > 0) {
            std::array<uint8_t, REROLLS> rerolls = get_best_rerolls(dice, player().scored_mask);
            uint8_t i = rerolls[fast_rand(REROLLS)];
            if (i >= lookup.sorted_rerolls.size()) {
                std::cerr << "Error: Index out of bounds!" << std::endl;
                std::cerr << "Index (i): " << (int)i << std::endl;
                std::cerr << "Size (lookup.sorted_rerolls.size()): " << lookup.sorted_rerolls.size() << std::endl;

                // Terminate the program
                std::exit(EXIT_FAILURE);
            }
            Reroll reroll = lookup.sorted_rerolls[i];
            dice.reroll(reroll);
            player().rerolls--;
        } else {
            const uint32_t never_cross = ~0b10000000000000111100;
            uint32_t i;
            if (player().scored_mask & never_cross) {
                i = (int)worst_category(player().scored_mask & never_cross);
            } else {
                i = (int)worst_category(player().scored_mask);
            }
            player().scored_mask ^= 1 << i;
            player().scores[i] = 0;
            next_player();
        }

        // move.type = Move::Type::Cross;
        // move.crossed_category = (Category)i;
        // move.type = Move::Type::Reroll;
        // move.reroll = lookup.rerolls[0];
        // std::cout << move.to_string() << std::endl;

        // std::cout << scores_string() << std::endl;
    }
}

std::string Game::scores_string() {
    std::ostringstream oss;
    oss << "================ SCOREBOARD ================\n";

    for (size_t p = 0; p < players.size(); ++p) {
        Player& player = players[p];
        int total = player.total_score();

        oss << "Player " << p << ":\n";
        for (int c = 0; c < (int)Category::Count; ++c) {
            oss << "  " << std::setw(15) << std::left << category_to_string((Category)c) << ": ";

            if (player.scores[c]) {
                oss << std::setw(3) << (int)*player.scores[c];
            } else {
                oss << "---";
            }
            oss << '\n';
        }

        oss << "  ------------------------------\n";
        oss << "  TOTAL: " << total << "\n\n";
    }
    return oss.str();
}
