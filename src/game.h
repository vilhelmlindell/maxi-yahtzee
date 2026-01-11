#ifndef GAME_HPP
#define GAME_HPP

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

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

const char* category_to_string(Category c);
Category category_from_string(std::string_view s);
bool is_bonus_category(Category c);

struct CategoryEntry {
    Category category = Category::Ones;
    uint8_t score = 0;

    bool operator==(const CategoryEntry&) const = default;
};

struct Player {
    std::array<std::optional<uint8_t>, (int)(Category::Count)> scores{};
    uint8_t bonus_progress = 0;
    uint32_t scored_mask = (1 << (int)Category::Count) - 1;
    uint8_t rerolls = 2;

    int total_score();
    void score(CategoryEntry entry);
};

struct Reroll {
    std::array<uint8_t, 6> hold_freq{};
    uint8_t num_rolls = 0;

    bool operator==(const Reroll&) const = default;
    std::string to_string();
};

struct Dice {
    std::array<uint8_t, 6> dice_freq{};

    Dice();
    Dice(std::array<uint8_t, 6> dice_freq);
    void reroll_all();
    void reroll(Reroll const& reroll);
    std::string to_string();
};

struct Move {
    enum class Type { Reroll, Score } type;
    Reroll reroll;
    CategoryEntry score_entry;

    Move();
    std::string to_string();
    Move from_string(const std::string& input);
    bool operator==(const Move&) const = default;
};

struct Game {
    inline static uint score_to_beat = 200;
    std::vector<Player> players;
    uint8_t player_i = 0;
    uint8_t rounds = 0;
    Dice dice;

    Game(int num_players);
    bool is_terminal();
    uint8_t winner();
    Player& player();
    void next_player();
    void play_move(Move move);
    Move random_move();
    void playout();
    std::string scores_string();
};

#endif // GAME_HPP
