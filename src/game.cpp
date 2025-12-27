#include "game.h"
#include "utils.h"
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <utility>
#include <iomanip>
#include <fstream>

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

uint8_t Player::total_score() {
    uint8_t sum = 0;
    for (const auto& s : scores) {
        if (s) {
            sum += *s;
        }
    }
    return sum;
}

bool CategoryEntry::operator==(const CategoryEntry& other) const {
    return category == other.category && score == other.score;
}

thread_local static std::vector<CategoriesLookup> categories_by_dice_vec = init_categories_by_dice_vec();

Dice::Dice() {
    reroll();
}

void Dice::reroll() {
    for (int i = 0; i < 6; i++) {
        dice[i] = (fast_rand(6)) + 1;
    }
}

void Dice::reroll(uint8_t mask) {
    for (int i = 0; i < 6; i++) {
        if (mask & (1 << i)) {
            dice[i] = (fast_rand(6)) + 1;
        }
    }
}

CategoriesLookup& Dice::categories() {
    std::array<uint8_t, 7> freq_arr{};
    for (uint8_t die : dice) {
        freq_arr[die] += 1;
    }
    size_t index = freq_to_index(freq_arr);
    return categories_by_dice_vec.at(index);
}

Move::Move() : type(Type::Reroll), reroll_mask(0), score_entry(), cross_i(0) {}

bool Move::operator==(const Move& other) const {
    if (type != other.type)
        return false;

    switch (type) {
    case Type::Reroll:
        return reroll_mask == other.reroll_mask;
    case Type::Cross:
        return cross_i == other.cross_i;
    case Type::Score:
        return score_entry == other.score_entry;
    }
    return false;
}

bool Move::operator!=(const Move& other) const {
    return !(*this == other);
}

std::string Move::to_string() const {
    static char buffer[64];

    switch (type) {
    case Move::Type::Reroll: {
        char bits[7];
        for (int i = 5; i >= 0; i--) {
            bits[5 - i] = (reroll_mask & (1 << i)) ? '1' : '0';
        }
        bits[6] = '\0';
        snprintf(buffer, sizeof(buffer), "reroll %s", bits);
        break;
    }
    case Move::Type::Cross: {
        snprintf(buffer, sizeof(buffer), "cross %u", cross_i);
        break;
    }
    case Move::Type::Score: {
        snprintf(buffer, sizeof(buffer), "score %s %d", category_to_string(score_entry.category), score_entry.score);
        break;
    }
    }

    return buffer;
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
        m.reroll_mask = static_cast<uint8_t>(mask);
    } else if (cmd == "cross") {
        uint32_t index = parse_uint(arg1);
        if (index > 0xFF)
            throw std::out_of_range("Cross index must fit in uint8_t");

        m.type = Move::Type::Cross;
        m.cross_i = static_cast<uint8_t>(index);
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
    dice.reroll();
    player().rerolls += 2;
}

void Game::play_move(Move move) {
    switch (move.type) {
    case Move::Type::Score: {
        CategoryEntry entry = move.score_entry;
        size_t i = (size_t)entry.category;
        player().scores[i] = entry.score;
        player().scored_mask ^= 1 << i;
        next_player();
        break;
    }
    case Move::Type::Reroll: {
        uint8_t mask = move.reroll_mask;
        dice.reroll(mask);
        player().rerolls--;
        break;
    }
    case Move::Type::Cross: {
        uint8_t i = move.cross_i;
        player().scores[i] = 0;
        player().scored_mask ^= 1 << i;
        next_player();
        break;
    }
    }
}

uint8_t Game::playout() {
    while (!is_terminal()) {
        if (fast_rand(2)) {
            CategoriesLookup lookup = dice.categories();
            CategoryEntry entry = lookup.categories[fast_rand(lookup.categories.size())];
            player().scores[(int)(entry.category)] = entry.score;
            next_player();
        } else if (player().rerolls > 0) {
            uint8_t mask = fast_rand((1 << 6) - 1) + 1;
            dice.reroll(mask);
            player().rerolls--;
        }
    }
    return winner();
}

std::string Game::scores_string() {
    std::ostringstream oss;
    oss << "================ SCOREBOARD ================\n";

    for (size_t p = 0; p < players.size(); ++p) {
        const Player& player = players[p];
        int total = 0;

        oss << "Player " << p << ":\n";
        for (int c = 0; c < (int)Category::Count; ++c) {
            oss << "  " << std::setw(15) << std::left << category_to_string((Category)c) << ": ";

            if (player.scores[c]) {
                oss << std::setw(3) << (int)*player.scores[c];
                total += *player.scores[c];
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

size_t freq_to_index(std::array<uint8_t, 7>& freq) {
    return freq[0] + freq[1] * 7 + freq[2] * 49 + freq[3] * 343 + freq[4] * 2401 + freq[5] * 16807;
}

std::vector<CategoriesLookup> init_categories_by_dice_vec() {
    const std::string filename = "categories_by_dice.dat";
    std::ifstream infile(filename, std::ios::binary);

    if (infile.is_open()) {
        size_t vector_size;
        infile.read(reinterpret_cast<char*>(&vector_size), sizeof(vector_size));
        std::vector<CategoriesLookup> categories_vec(vector_size);

        for (auto& lookup : categories_vec) {
            infile.read(reinterpret_cast<char*>(&lookup.category_mask), sizeof(lookup.category_mask));
            size_t categories_size;
            infile.read(reinterpret_cast<char*>(&categories_size), sizeof(categories_size));
            lookup.categories.resize(categories_size);
            infile.read(reinterpret_cast<char*>(lookup.categories.data()), categories_size * sizeof(CategoryEntry));
        }
        infile.close();
        return categories_vec;
    }

    std::vector<CategoriesLookup> categories_vec(117649);

    // clang-format off
    for (uint8_t a1 = 0; a1 <= 6; ++a1) {
    for (uint8_t a2 = 0; a2 <= 6 - a1; ++a2) {
    for (uint8_t a3 = 0; a3 <= 6 - a1 - a2; ++a3) {
    for (uint8_t a4 = 0; a4 <= 6 - a1 - a2 - a3; ++a4) {
    for (uint8_t a5 = 0; a5 <= 6 - a1 - a2 - a3 - a4; ++a5) {
        uint8_t a6 = 6 - a1 - a2 - a3 - a4 - a5;
        std::array<uint8_t, 7> freq_arr = {0, a1, a2, a3, a4, a5, a6};
        CategoriesLookup categories = dice_categories(freq_arr);
        size_t index = freq_to_index(freq_arr);
        categories_vec[index] = std::move(categories);
    } } } } }
    // clang-format on

    std::ofstream outfile(filename, std::ios::binary);
    if (outfile.is_open()) {
        size_t vector_size = categories_vec.size();
        outfile.write(reinterpret_cast<const char*>(&vector_size), sizeof(vector_size));

        for (const auto& lookup : categories_vec) {
            outfile.write(reinterpret_cast<const char*>(&lookup.category_mask), sizeof(lookup.category_mask));
            size_t categories_size = lookup.categories.size();
            outfile.write(reinterpret_cast<const char*>(&categories_size), sizeof(categories_size));
            outfile.write(reinterpret_cast<const char*>(lookup.categories.data()), categories_size * sizeof(CategoryEntry));
        }
        outfile.close();
    }

    return categories_vec;
}

CategoriesLookup dice_categories(std::array<uint8_t, 7> freq_arr) {
    std::vector<CategoryEntry> categories;
    for (int num = 1; num <= 6; num++) {
        uint8_t freq = freq_arr[num];
        if (freq != 0) {
            CategoryEntry entry;
            entry.category = (Category)(num - 1);
            entry.score = freq * num;
            categories.push_back(entry);
        }
    }

    std::vector<int> pairs;
    std::vector<int> threes;
    int four_kind = 0;
    int five_kind = 0;

    for (int num = 1; num <= 6; num++) {
        uint8_t freq = freq_arr[num];
        if (freq >= 2)
            pairs.push_back(num);
        if (freq >= 3)
            threes.push_back(num);
        if (freq >= 4)
            four_kind = num;
        if (freq >= 5)
            five_kind = num;
    }

    if (!pairs.empty()) {
        CategoryEntry entry;
        entry.category = Category::Pair;
        entry.score = 2 * pairs.back();
        categories.push_back(entry);
    }

    if (pairs.size() >= 2) {
        CategoryEntry entry;
        entry.category = Category::TwoPair;
        entry.score = 2 * pairs[pairs.size() - 1] + 2 * pairs[pairs.size() - 2];
        categories.push_back(entry);
    }

    if (pairs.size() >= 3) {
        CategoryEntry entry;
        entry.category = Category::ThreePair;
        entry.score = 2 * (pairs[pairs.size() - 1] + pairs[pairs.size() - 2] + pairs[pairs.size() - 3]);
        categories.push_back(entry);
    }

    if (!threes.empty()) {
        CategoryEntry entry;
        entry.category = Category::ThreeKind;
        entry.score = 3 * threes.back();
        categories.push_back(entry);
    }

    if (four_kind > 0) {
        CategoryEntry entry;
        entry.category = Category::FourKind;
        entry.score = 4 * four_kind;
        categories.push_back(entry);
    }

    if (five_kind > 0) {
        CategoryEntry entry;
        entry.category = Category::FiveKind;
        entry.score = 5 * five_kind;
        categories.push_back(entry);
    }

    int consecutive = 0;
    int max_consecutive = 0;
    for (int num = 1; num <= 6; num++) {
        if (freq_arr[num] > 0) {
            consecutive++;
            max_consecutive = std::max(max_consecutive, consecutive);
        } else {
            consecutive = 0;
        }
    }

    if (max_consecutive >= 4) {
        CategoryEntry entry;
        entry.category = Category::SmallStraight;
        entry.score = 30;
        categories.push_back(entry);
    }

    if (max_consecutive >= 5) {
        CategoryEntry entry;
        entry.category = Category::LargeStraight;
        entry.score = 40;
        categories.push_back(entry);
    }

    if (max_consecutive == 6) {
        CategoryEntry entry;
        entry.category = Category::Straight;
        entry.score = 50;
        categories.push_back(entry);
    }

    if (threes.size() >= 1 && pairs.size() >= 2) {
        CategoryEntry entry;
        entry.category = Category::House;
        int sum = 0;
        for (int num = 1; num <= 6; num++) {
            sum += freq_arr[num] * num;
        }
        entry.score = sum;
        categories.push_back(entry);
    }

    if (threes.size() >= 2) {
        CategoryEntry entry;
        entry.category = Category::Villa;
        int sum = 0;
        for (int num = 1; num <= 6; num++) {
            sum += freq_arr[num] * num;
        }
        entry.score = sum;
        categories.push_back(entry);
    }

    if (four_kind > 0 && pairs.size() >= 2) {
        CategoryEntry entry;
        entry.category = Category::Tower;
        int sum = 0;
        for (int num = 1; num <= 6; num++) {
            sum += freq_arr[num] * num;
        }
        entry.score = sum;
        categories.push_back(entry);
    }

    {
        CategoryEntry entry;
        entry.category = Category::Chance;
        int sum = 0;
        for (int num = 1; num <= 6; num++) {
            sum += freq_arr[num] * num;
        }
        entry.score = sum;
        categories.push_back(entry);
    }

    if (five_kind > 0 && freq_arr[five_kind] == 6) {
        CategoryEntry entry;
        entry.category = Category::MaxiYahtzee;
        entry.score = 100;
        categories.push_back(entry);
    }
    CategoriesLookup lookup;
    lookup.categories = categories;
    for (auto& entry : categories) {
        lookup.category_mask |= 1 << (int)entry.category;
    }
    return lookup;
}
