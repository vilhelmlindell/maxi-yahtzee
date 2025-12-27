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

struct Player {
    std::array<std::optional<uint8_t>, (int)(Category::Count)> scores{};
    uint32_t scored_mask = (1 << (int)Category::Count) - 1;
    uint8_t rerolls = 2;

    uint8_t total_score();
};

struct CategoryEntry {
    Category category = Category::Ones;
    uint8_t score = 0;

    bool operator==(const CategoryEntry& other) const;
};

struct CategoriesLookup {
    std::vector<CategoryEntry> categories;
    uint32_t category_mask = 0;
};

struct Dice {
    std::array<uint8_t, 6> dice{};

    Dice();
    void reroll();
    void reroll(uint8_t mask);
    CategoriesLookup& categories();
};

struct Move {
    enum class Type { Reroll, Cross, Score } type;
    uint8_t reroll_mask = 0;
    CategoryEntry score_entry;
    uint8_t cross_i;

    Move();
    bool operator==(const Move& other) const;
    bool operator!=(const Move& other) const;
    std::string to_string() const;
    static Move from_string(const std::string& input);
};

struct Game {
    inline static uint8_t score_to_beat = 200;
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
    uint8_t playout();
    std::string scores_string();
};

std::vector<CategoriesLookup> init_categories_by_dice_vec();
CategoriesLookup dice_categories(std::array<uint8_t, 7> freq_arr);
size_t freq_to_index(std::array<uint8_t, 7>& freq);

#endif // GAME_HPP
