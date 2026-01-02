#pragma once


#include "game.h"
#include <array>
#include <cstdint>

extern std::array<int, (int)Category::Count> max_scores;
extern std::array<double, (int)Category::Count> avg_scores;
extern std::array<double, (int)Category::Count> expected_values;

struct DiceLookup {
    std::vector<CategoryEntry> categories;
    std::array<Reroll, 63> rerolls;
    std::array<std::array<float, (int)Category::Count>, 63> reroll_category_evs;
    uint32_t category_mask = 0;
    // Unique index for each dice combination from 0 to 461
    uint16_t index;
};

struct MaskLookup {
    std::array<Reroll, 462> best_rerolls;
    uint8_t worst_category;
};

void init_dice_lookups();
void init_mask_lookups();
Reroll get_best_reroll(Dice dice, uint32_t mask);
DiceLookup& get_dice_lookup(Dice dice);
double get_score_heuristic(CategoryEntry entry);
void calculate_global_category_evs();
