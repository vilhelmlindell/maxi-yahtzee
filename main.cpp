#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

using namespace std;

thread_local uint32_t rng_state = 321321321;

inline uint32_t xorshift32() {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}
//static inline uint32_t xorshift32(void) {
//    rng_state = rng_state * 1664525u + 1013904223u;
//    return rng_state & 7u; // 3 bits
//}

inline uint32_t fast_rand(uint32_t max) {
    return xorshift32() % max;
}

uint32_t random_set_bit(uint32_t x) {
    uint32_t start = rand() & 31;
    uint32_t y = (x >> start) | (x << (32 - start));
    return ((__builtin_ctz(y) + start) & 31);
}

inline uint32_t parse_uint(const std::string& s) {
    size_t idx = 0;
    uint32_t value = std::stoul(s, &idx, 0); // base=0 → auto-detect
    if (idx != s.size())
        throw std::invalid_argument("Invalid integer: " + s);
    return value;
}

enum class Category : uint8_t {
    Ones,
    Twos,
    Threes,
    Fours,
    Fives,
    Sixes,
    Pair,
    TwoPair,
    ThreePair,
    ThreeKind,
    FourKind,
    FiveKind,
    SmallStraight,
    LargeStraight,
    Straight,
    House,
    Villa,
    Tower,
    Chance,
    MaxiYahtzee,
    Count,
};

inline const char* string_from_category(Category c) {
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

inline Category category_from_string(std::string_view s) {
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

struct Player {
    array<optional<uint8_t>, (int)(Category::Count)> scores{};
    uint32_t scored_mask = (1 << (int)Category::Count) - 1;
    uint8_t rerolls = 2;
};

struct CategoryEntry {
    Category category = Category::Ones;
    uint8_t score = 0;

    bool operator==(const CategoryEntry& other) const {
        return category == other.category && score == other.score;
    }
};

struct CategoriesLookup {
    vector<CategoryEntry> categories;
    uint32_t category_mask;
};

CategoriesLookup dice_categories(array<uint8_t, 7> freq_arr) {
    vector<CategoryEntry> categories;
    for (int num = 1; num <= 6; num++) {
        uint8_t freq = freq_arr[num];
        if (freq != 0) {
            CategoryEntry entry;
            entry.category = (Category)(num - 1);
            entry.score = freq * num;
            categories.push_back(entry);
        }
    }

    vector<int> pairs;
    vector<int> threes;
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
            max_consecutive = max(max_consecutive, consecutive);
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

inline size_t freq_to_index(array<uint8_t, 7>& freq) {
    return freq[0] + freq[1] * 7 + freq[2] * 49 + freq[3] * 343 + freq[4] * 2401 + freq[5] * 16807;
}

vector<CategoriesLookup> init_categories_by_dice_vec() {
    vector<CategoriesLookup> categories_vec(117649); // +1 for safety

    // clang-format off
    for (uint8_t a1 = 0; a1 <= 6; ++a1) {
    for (uint8_t a2 = 0; a2 <= 6 - a1; ++a2) {
    for (uint8_t a3 = 0; a3 <= 6 - a1 - a2; ++a3) {
    for (uint8_t a4 = 0; a4 <= 6 - a1 - a2 - a3; ++a4) {
    for (uint8_t a5 = 0; a5 <= 6 - a1 - a2 - a3 - a4; ++a5) {
        uint8_t a6 = 6 - a1 - a2 - a3 - a4 - a5;
        array<uint8_t, 7> freq_arr = {0, a1, a2, a3, a4, a5, a6};
        CategoriesLookup categories = dice_categories(freq_arr);
        size_t index = freq_to_index(freq_arr);
        categories_vec[index] = std::move(categories);
    } } } } }
    // clang-format on

    return categories_vec;
}

thread_local static vector<CategoriesLookup> categories_by_dice_vec = init_categories_by_dice_vec();

// thread_local array<uint8_t, 63> reroll_masks = []() {
//     array<uint8_t, 63> masks;
//     for (int i = 0; i < 63; i++) {
//         masks[i] = i + 1;
//     }
//     return masks;
// }();
// thread_local size_t reroll_i = reroll_masks.size() - 1;

struct Dice {
    array<uint8_t, 6> dice{};

    Dice() {
        reroll();
    }

    void reroll() {
        for (int i = 0; i < 6; i++) {
            dice[i] = (fast_rand(6)) + 1;
        }
    }
    void reroll(uint8_t mask) {
        for (int i = 0; i < 6; i++) {
            if (mask & (1 << i)) {
                dice[i] = (fast_rand(6)) + 1;
            }
        }
    }

    CategoriesLookup& categories() {
        array<uint8_t, 7> freq_arr{};
        for (uint8_t die : dice) {
            freq_arr[die] += 1;
        }
        size_t index = freq_to_index(freq_arr);
        return categories_by_dice_vec.at(index);
    }
};

struct Move {
    enum class Type { Reroll, Cross, Score } type;
    uint8_t reroll_mask = 0;
    CategoryEntry score_entry;
    uint8_t cross_i;

    Move()
        : score_entry() {}
    // Move(string str) {
    //     *this = from_string(str);
    // }

    bool operator==(const Move& other) const {
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

    bool operator!=(const Move& other) const {
        return !(*this == other);
    }

    string to_string() const {
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
            snprintf(buffer, sizeof(buffer), "score %s %d", string_from_category(score_entry.category), score_entry.score);
            break;
        }
        }

        return buffer;
    }

    Move from_string(const std::string& input) {
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
};

struct Game {
    vector<Player> players;
    uint8_t player_i = 0;
    uint8_t rounds = 0;
    Dice dice;

    Game(int num_players) {
        for (int i = 0; i < num_players; i++) {
            players.push_back(Player());
        }
    }
    bool is_terminal() {
        return rounds == (int)(Category::Count);
    }
    uint8_t winner() {
        vector<uint8_t> scores = vector<uint8_t>(players.size());
        for (size_t i = 0; i < players.size(); i++) {
            for (const auto& s : players[i].scores) {
                if (s) {
                    scores[i] += *s;
                }
            }
        }
        auto it = std::max_element(scores.begin(), scores.end());
        return std::distance(scores.begin(), it);
    }
    Player& player() {
        return players[player_i];
    }
    void next_player() {
        player_i++;
        if (player_i == players.size()) {
            player_i = 0;
            rounds++;
        }
        dice.reroll();
        player().rerolls += 2;
    }
    void play_move(Move move) {
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
    uint8_t playout() {
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
};

struct MCTSNode {
    Game game;
    vector<unique_ptr<MCTSNode>> children;
    MCTSNode* parent = nullptr;
    optional<Move> move;

    uint8_t player_i = 0;
    uint32_t visits = 0;
    uint32_t wins = 0;

    uint8_t rerolls_left = 32;
    uint8_t fisher_i = 0;

    vector<CategoryEntry>* categories = nullptr;
    uint32_t score_mask = 0;
    uint32_t cross_mask = 0;
    bool rerolls_done = false;
    optional<CategoryEntry> next_category;

    MCTSNode(Game game)
        : game(game) {
        player_i = game.player_i;

        categories = &game.dice.categories().categories;
        score_mask = game.dice.categories().category_mask & game.player().scored_mask;
        cross_mask = game.player().scored_mask;
        fisher_i = categories->size();

        next_category = find_next_category();
    }

    double uct() const {
        if (visits == 0)
            return INFINITY;
        if (parent == nullptr)
            return 0.0;

        const double C = 1.414;
        return (double)wins / (double)visits + C * sqrt(log(parent->visits) / visits);
    }

    bool is_terminal() {
        return game.is_terminal();
    }

    bool is_leaf_node() {
        return cross_mask || (!rerolls_done && game.player().rerolls > 0) || next_category.has_value();
    }

    MCTSNode* select_child() {
        MCTSNode* current = this;

        while (!current->is_leaf_node()){
            auto selected = max_element(current->children.begin(), current->children.end(), [](const unique_ptr<MCTSNode>& a, const unique_ptr<MCTSNode>& b) {
                return a->uct() < b->uct();
            });
            current = selected->get();
        }

        return current;
    }

    MCTSNode* expand() {
        if (is_terminal()) {
            return this;
        }

        Game new_game = game;
        Move move = next_move();
        new_game.play_move(move);

        unique_ptr<MCTSNode> child = make_unique<MCTSNode>(new_game);
        child->parent = this;
        child->move = move;
        child->player_i = player_i;

        children.push_back(std::move(child));

        return children.back().get();
    }

    uint8_t simulate() {
        Game sim_game = game;
        return sim_game.playout();
    }

    void backpropagate(uint8_t winner) {
        visits++;
        if (player_i == winner) {
            wins++;
        }

        if (parent != nullptr) {
            parent->backpropagate(winner);
        }
    }

    void run_iteration() {
        MCTSNode* leaf = select_child();
        MCTSNode* node = leaf->expand();
        uint8_t winner = node->simulate();
        node->backpropagate(winner);
    }

    optional<CategoryEntry> find_next_category() {
        while (fisher_i > 0) {
            fisher_i--;

            size_t i = fast_rand(fisher_i + 1);
            swap(categories->at(i), categories->at(fisher_i));

            CategoryEntry& entry = categories->at(fisher_i);
            uint32_t bit = 1u << (uint32_t)entry.category;

            if (score_mask & bit) {
                return entry;
            }
        }
        return std::nullopt;
    }

    Move next_move() {
        Move move;

        if (next_category.has_value()) {
            move.type = Move::Type::Score;
            move.score_entry = next_category.value();

            next_category = find_next_category();
        } else if (!rerolls_done && game.player().rerolls > 0) {
            rerolls_left--;
            rerolls_done = (rerolls_left == 0);

            uint8_t mask = fast_rand(63) + 1;

            move.type = Move::Type::Reroll;
            move.reroll_mask = mask;
        } else if (cross_mask) {
            uint32_t i = random_set_bit(cross_mask);
            if (i > 20) {
                cout << "Hmmmm: " << cross_mask << endl;
            }
            move.type = Move::Type::Cross;
            move.cross_i = i;
            cross_mask ^= 1 << i;
        } else {
            cerr << "panic: next_move found no move\n";
            terminate();
        }
        return move;
    }

    MCTSNode* best_child() const {
        if (children.empty())
            return nullptr;

        auto best = max_element(children.begin(), children.end(), [](const unique_ptr<MCTSNode>& a, const unique_ptr<MCTSNode>& b) {
            return a->visits < b->visits;
        });

        return best->get();
    }

    void write_dot(std::ofstream& out, int& node_id, int parent_id = -1, int max_depth = 3, int current_depth = 0) {
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

    void save_tree(const char* filename, int max_depth = 3) {
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

    std::string to_string() {
        std::ostringstream oss;

        double winrate = visits > 0 ? (double)wins / (double)visits : 0.0;

        oss << "Visits: " << visits << " | Wins: " << wins << " | Winrate: " << std::fixed << std::setprecision(4) << winrate << " | UCT: " << std::setprecision(4) << uct()
            << " | ";

        if (move) {
            oss << "Move: " << move.value().to_string();
        } else {
            oss << "Move: None";
        }

        return oss.str();
    }
};

Move run_mcts(Game game) {
    unsigned int n_threads = std::thread::hardware_concurrency();
    vector<unique_ptr<MCTSNode>> thread_roots;
    vector<thread> threads;

    auto duration = std::chrono::milliseconds(100);
    for (uint i = 0; i < n_threads; i++) {
        thread_roots.push_back(make_unique<MCTSNode>(game));
        threads.push_back(thread([&, i]() {
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
    for (uint i = 0; i < n_threads - 1; i++) {
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

    std::sort(total->children.begin(), total->children.end(), [](const unique_ptr<MCTSNode>& a, const unique_ptr<MCTSNode>& b) {
        return a->visits > b->visits;
    });

    for (auto& node : total->children) {
        cout << node->to_string() << std::endl;
    }

    return total->children[0]->move.value();
}

void print_scores(const Game& game) {
    cout << "================ SCOREBOARD ================\n";

    for (size_t p = 0; p < game.players.size(); ++p) {
        const Player& player = game.players[p];
        int total = 0;

        cout << "Player " << p << ":\n";
        for (int c = 0; c < (int)Category::Count; ++c) {
            cout << "  " << setw(15) << left << string_from_category((Category)c) << ": ";

            if (player.scores[c]) {
                cout << setw(3) << (int)*player.scores[c];
                total += *player.scores[c];
            } else {
                cout << "---";
            }
            cout << '\n';
        }

        cout << "  ------------------------------\n";
        cout << "  TOTAL: " << total << "\n\n";
    }
}

void play_game(Game game) {
    while (!game.is_terminal()) {
        Move move = run_mcts(game);

        // cout << "\nPlayer " << (int)game.player_i
        //      << " plays: " << move.to_string() << "\n";

        cout << "Player: " << (int)game.player_i << " " << move.to_string() << endl;
        if (move.type != Move::Type::Reroll) {
            cout << endl;
        }

        game.play_move(move);
        print_scores(game);
    }

    printf("Winner: %d\n", game.winner());
}

void distribution() {
    uint32_t mask = 0b10000000100000001000000010000000;
    array<int, 32> freq_arr{};
    for (int i = 0; i < 10000; i++) {
        uint32_t j = random_set_bit(mask);
        freq_arr[j]++;
    }
    for (int i = 0; i < 32; i++) {
        cout << "i: " << i << " freq: " << freq_arr[i] << endl;
    }
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));
    rng_state = rand();

    Game game = Game(2);
    game.dice.dice = {6, 3, 6, 5, 5, 4};

    play_game(game);

    //distribution();
    //MCTSNode root = MCTSNode(game);
    //for (int i = 0; i < 200; i++) {
    //    root.run_iteration();
    //}
    //cout << root.children.size() << endl;

    //std::sort(root.children.begin(), root.children.end(), [](const unique_ptr<MCTSNode>& a, const unique_ptr<MCTSNode>& b) {
    //    return a->visits > b->visits;
    //});

    //for (auto& node : root.children) {
    //    cout << node->to_string() << std::endl;
    //}

    //cout << root.children[0]->move.value().to_string() << endl;
}
