#pragma once


#include "game.h"
#include <array>
#include <cstdint>

extern std::array<int, (int)Category::Count> max_scores;
extern std::array<double, (int)Category::Count> avg_scores;
extern std::array<double, (int)Category::Count> expected_values;

struct DiceLookup {
    std::vector<CategoryEntry> categories;
    //std::array<Reroll, 63> rerolls;
    std::vector<Reroll> sorted_rerolls;
    std::vector<std::array<float, (int)Category::Count>> reroll_category_evs;
    uint32_t category_mask = 0;
    // Unique index for each dice combination from 0 to 461
    uint16_t index;
};

const int REROLLS = 6;

struct MaskLookup {
    std::array<std::array<uint8_t, REROLLS>, 462> best_rerolls;
    uint8_t worst_category;
};

void init_dice_lookups();
void init_mask_lookups();
std::array<uint8_t, REROLLS> get_best_rerolls(Dice dice, uint32_t mask);
DiceLookup& get_dice_lookup(Dice dice);
double get_score_heuristic(CategoryEntry entry);
void calculate_global_category_evs();
