#include "lookup.h"
#include "game.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <random>

const std::string CACHE_FILENAME = "dice_lookups.bin";
const std::string MASK_CACHE_FILENAME = "mask_lookups.bin";

std::vector<DiceLookup> dice_lookups(117649);
std::vector<MaskLookup> mask_lookups(1 << 20);

std::array<Dice, 462> all_dice;
std::array<std::array<uint32_t, 12>, 12> pascals;

static void save_binary(const std::string& filename, bool is_mask_table = false) {
    // Check if file already exists
    {
        std::ifstream check(filename);
        if (check.good()) {
            std::cout << "File " << filename << " already exists. Skipping save." << std::endl;
            return;
        }
    }

    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) {
        std::cerr << "Could not open file for writing: " << filename << std::endl;
        return;
    }

    if (is_mask_table) {
        size_t total_entries = mask_lookups.size();
        ofs.write(reinterpret_cast<const char*>(&total_entries), sizeof(total_entries));
        // MaskLookup is POD-stable (contains fixed array of Reroll and a uint8_t)
        ofs.write(reinterpret_cast<const char*>(mask_lookups.data()), total_entries * sizeof(MaskLookup));
    } else {
        size_t total_entries = dice_lookups.size();
        ofs.write(reinterpret_cast<const char*>(&total_entries), sizeof(total_entries));

        for (const auto& entry : dice_lookups) {
            // 1. Save Category Vector
            size_t vec_size = entry.categories.size();
            ofs.write(reinterpret_cast<const char*>(&vec_size), sizeof(vec_size));
            if (vec_size > 0) {
                ofs.write(reinterpret_cast<const char*>(entry.categories.data()), vec_size * sizeof(CategoryEntry));
            }

            // 2. Save Fixed Arrays (64 entries each)
            ofs.write(reinterpret_cast<const char*>(entry.rerolls.data()), entry.rerolls.size() * sizeof(Reroll));
            ofs.write(reinterpret_cast<const char*>(entry.sorted_rerolls.data()), entry.sorted_rerolls.size() * sizeof(Reroll));
            
            // 3. Save EV Table (2D Array)
            ofs.write(reinterpret_cast<const char*>(entry.reroll_category_evs.data()), 
                      entry.reroll_category_evs.size() * sizeof(std::array<float, (int)Category::Count>));

            // 4. Save Primitives
            ofs.write(reinterpret_cast<const char*>(&entry.category_mask), sizeof(entry.category_mask));
            ofs.write(reinterpret_cast<const char*>(&entry.index), sizeof(entry.index));
        }
    }
    std::cout << "Table saved to " << filename << std::endl;
}

static bool load_binary(const std::string& filename, bool is_mask_table = false) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) return false;

    size_t total_entries;
    ifs.read(reinterpret_cast<char*>(&total_entries), sizeof(total_entries));

    if (is_mask_table) {
        if (total_entries != mask_lookups.size()) return false;
        ifs.read(reinterpret_cast<char*>(mask_lookups.data()), total_entries * sizeof(MaskLookup));
    } else {
        if (total_entries != dice_lookups.size()) return false;
        for (auto& entry : dice_lookups) {
            // 1. Load Category Vector
            size_t vec_size;
            ifs.read(reinterpret_cast<char*>(&vec_size), sizeof(vec_size));
            entry.categories.resize(vec_size);
            if (vec_size > 0) {
                ifs.read(reinterpret_cast<char*>(entry.categories.data()), vec_size * sizeof(CategoryEntry));
            }

            // 2. Load Fixed Arrays
            ifs.read(reinterpret_cast<char*>(entry.rerolls.data()), entry.rerolls.size() * sizeof(Reroll));
            ifs.read(reinterpret_cast<char*>(entry.sorted_rerolls.data()), entry.sorted_rerolls.size() * sizeof(Reroll));

            // 3. Load EV Table
            ifs.read(reinterpret_cast<char*>(entry.reroll_category_evs.data()), 
                     entry.reroll_category_evs.size() * sizeof(std::array<float, (int)Category::Count>));

            // 4. Load Primitives
            ifs.read(reinterpret_cast<char*>(&entry.category_mask), sizeof(entry.category_mask));
            ifs.read(reinterpret_cast<char*>(&entry.index), sizeof(entry.index));
        }
    }
    return true;
}

std::array<int, (int)Category::Count> max_scores = {
    6,  // ONES
    12, // TWOS
    18, // THREES
    24, // FOURS
    30, // FIVES
    36, // SIXES
    12, // PAIR
    22, // TWO_PAIR
    30, // THREE_PAIR
    18, // THREE_KIND
    24, // FOUR_KIND
    30, // FIVE_KIND
    15, // SMALL_STRAIGHT
    20, // LARGE_STRAIGHT
    21, // STRAIGHT
    28, // HOUSE
    33, // VILLA
    34, // TOWER
    36, // CHANCE
    100 // MAXI_YAHTZEE
};
std::array<double, (int)Category::Count> avg_scores = {
    1,       // ONES
    4,       // TWOS
    12,      // THREES
    16,      // FOURS
    20,      // FIVES
    24,      // SIXES
    8.44252, // PAIR
    14.2545, // TWO_PAIR
    21.0,    // THREE_PAIR
    10.6636, // THREE_KIND
    14.0,    // FOUR_KIND
    17.5,    // FIVE_KIND
    15.0,    // SMALL_STRAIGHT
    20.0,    // LARGE_STRAIGHT
    21.0,    // STRAIGHT
    21.0,    // HOUSE
    21.0,    // VILLA
    21.0,    // TOWER
    21.0,    // CHANCE
    100.0    // MAXI_YAHTZEE
};

std::array<double, (int)Category::Count> expected_values = {1.0,      2.0,       3.0,      4.0,     5.0,      6.0,     8.23478,  7.92181,  0.810185, 3.87899,
                                                            0.730967, 0.0697659, 0.810185, 1.08025, 0.324074, 3.57832, 0.135031, 0.202546, 21.0,     0.0128601};

static size_t freq_to_index(std::array<uint8_t, 6> const& dice_freq) {
    return dice_freq[0] + dice_freq[1] * 7 + dice_freq[2] * 7 * 7 + dice_freq[3] * 7 * 7 * 7 + dice_freq[4] * 7 * 7 * 7 * 7 + dice_freq[5] * 7 * 7 * 7 * 7 * 7;
}


static Reroll compute_best_reroll(Dice dice, uint32_t mask) {
    DiceLookup& lookup = get_dice_lookup(dice);
    Reroll best;
    float best_score = 0;
    int bonus_progress = 0;
    for (int i = (int)Category::Ones; i <= (int)Category::Sixes; i++) {
        bonus_progress += avg_scores[i];
    }
    int rounds = std::popcount(mask);
    for (int i = 0; i < 63; i++) {
        //std::cout << lookup.sorted_rerolls[i].to_string() << std::endl;
        float score = 0;
        for (int j = 0; j < 20; j++) {
            //if (j >= (int)Category::Threes && j <= (int)Category::Fours) {
            //    (rounds / 20) * (75 / bonus_progress);
            //}
            if (mask & (1 << j)) {
                score += lookup.reroll_category_evs[i][j];
            }
        }
        if (score > best_score) {
            best_score = score;
            best = lookup.sorted_rerolls[i];
        }
    }
    return best;
}

DiceLookup& get_dice_lookup(Dice dice) {
    return dice_lookups[freq_to_index(dice.dice_freq)];
}

Reroll get_best_reroll(Dice dice, uint32_t mask) {
    DiceLookup lookup = get_dice_lookup(dice);
    //return compute_best_reroll(dice, mask);
    return mask_lookups[mask].best_rerolls[lookup.index];
}

double get_score_heuristic(CategoryEntry entry) {
    int max = max_scores[(int)entry.category];
    bool is_upper = (int)entry.category >= (int)Category::Threes && (int)entry.category <= (int)Category::Sixes;
    if (is_upper) {
        if ((double)entry.score >= avg_scores[(int)entry.category]) {
            return 1.0;
        } else {
            return 0.0;
        }
    }
    return (double)entry.score * (double)entry.score / ((double)max * (double)max);
    // double avg = avg_scores[(int)entry.category];
}

// double get_score_heuristic(CategoryEntry entry) {
//     double avg = avg_scores[(int)entry.category];
//
//     // 1. Calculate Efficiency (Relative to average)
//     // If avg is 0 (shouldn't happen here), handle to avoid nan
//     double efficiency = (avg > 0) ? (double)entry.score / avg : 0.0;
//
//     // 2. Bonus Weighting for Upper Section (Ones through Sixes)
//     // The "Bonus Potential" of a category is its contribution to the 75 limit.
//     // We give a slight edge to higher numbers because they are the "heavy lifters."
//     double bonus_weight = 1.0;
//     bool is_upper = (int)entry.category >= (int)Category::Ones &&
//                     (int)entry.category <= (int)Category::Sixes;
//
//     if (is_upper) {
//         // Higher upper categories are more valuable for securing the bonus.
//         // We add a weight to reflect their contribution to the 75-point goal.
//         bonus_weight = 1.0 + ((int)entry.category * 0.1);
//     }
//
//     // 3. Combine Score and Efficiency
//     // We use a weighted sum:
//     // - The actual score (absolute value)
//     // - The efficiency (how 'maximized' this slot is)
//     // - The bonus_weight (importance of the slot)
//
//     double heuristic_value = (entry.score * 0.7) + (efficiency * 10.0 * bonus_weight);
//
//     return heuristic_value;
// }

static void init_outcomes(int num_rolls, std::array<uint8_t, 6> current_freq, double prob, std::map<std::array<uint8_t, 6>, double>& outcomes) {
    if (num_rolls == 0) {
        outcomes[current_freq] += prob;
        return;
    }

    for (int die = 0; die < 6; ++die) {
        std::array<uint8_t, 6> next_freq = current_freq;
        next_freq[die]++;
        init_outcomes(num_rolls - 1, next_freq, prob * (1.0 / 6.0), outcomes);
    }
}

static std::array<uint8_t, 6> freq_to_flat(std::array<uint8_t, 6> const& freq_dice) {
    std::array<uint8_t, 6> flat_dice{};
    int count = 0;
    for (int i = 0; i < 6; i++) {
        for (int j = 0; j < freq_dice[i]; j++) {
            flat_dice[count] = i;
            count++;
        }
    }
    return flat_dice;
}

// static std::array<uint8_t, 6> freq_to_flat(std::array<uint8_t, 6> const& freq_dice) {
//     std::array<uint8_t, 6> flat_dice{};
//     int count = 0;
//     for (int i = 0; i < 6; i++) {
//         for (int j = 0; j < freq_dice[i]; j++) {
//             flat_dice[count] = i;
//             count++;
//         }
//     }
//     return flat_dice;
// }

static void init_ev(std::array<uint8_t, 6>& dice_freq, DiceLookup& lookup) {
    std::array<uint8_t, 6> flat_dice = freq_to_flat(dice_freq);

    std::array<std::tuple<Reroll, double, std::array<float, (int)Category::Count>>, 63> reroll_ev_sums;

    for (int i = 0; i < 63; i++) {
        Reroll reroll;
        reroll.hold_freq = {};
        reroll.num_rolls = 0;

        for (int j = 0; j < 6; ++j) {
            if ((i >> j) & 1) {
                reroll.hold_freq[flat_dice[j]]++;
            } else {
                reroll.num_rolls++;
            }
        }

        lookup.rerolls[i] = reroll;

        std::map<std::array<uint8_t, 6>, double> outcomes;
        init_outcomes(reroll.num_rolls, reroll.hold_freq, 1.0, outcomes);

        std::array<float, (int)Category::Count> category_evs{};
        double ev_sum = 0;

        for (auto const& [result_dice, probability] : outcomes) {
            Dice dice;
            dice.dice_freq = result_dice;
            DiceLookup& result_lookup = get_dice_lookup(dice);
            for (auto const& entry : result_lookup.categories) {
                double ev = entry.score * probability;
                if ((int)entry.category >= category_evs.size()) {
                    std::cout << "Error: " << (int)entry.category << std::endl;
                }
                category_evs[(int)entry.category] += entry.score * probability;
                ev_sum += ev;
            }
            // for (auto const& dice : result_dice) {
            //     std::cout << (int)dice << " ";
            // }
            //  std::cout << probability << std::endl;
        }
        reroll_ev_sums[i] = std::make_tuple(reroll, ev_sum, category_evs);
    }
    std::sort(reroll_ev_sums.begin(), reroll_ev_sums.end(), [](const auto& a, const auto& b) {
        return std::get<1>(a) > std::get<1>(b); // descending by double
    });

    for (std::size_t i = 0; i < reroll_ev_sums.size(); ++i) {
        lookup.sorted_rerolls[i] = std::get<0>(reroll_ev_sums[i]);
        lookup.reroll_category_evs[i] = std::get<2>(reroll_ev_sums[i]);
    }
}

static void init_categories(std::array<uint8_t, 6>& dice_freq, DiceLookup& lookup) {
    std::vector<CategoryEntry> categories;

    // 1. Individual Number Categories (Ones - Sixes)
    int total_sum = 0;
    for (int i = 0; i < 6; i++) {
        int die_val = i + 1;
        int count = dice_freq[i];
        total_sum += count * die_val;
        if (count > 0) {
            categories.push_back({(Category)i, (uint8_t)(count * die_val)});
        }
    }

    // 2. Count Patterns (Pairs, Threes, etc.)
    std::vector<int> pairs, threes;
    int four_kind = 0, five_kind = 0, six_kind = 0;

    for (int i = 0; i < 6; i++) {
        int val = i + 1;
        if (dice_freq[i] >= 2)
            pairs.push_back(val);
        if (dice_freq[i] >= 3)
            threes.push_back(val);
        if (dice_freq[i] >= 4)
            four_kind = val;
        if (dice_freq[i] >= 5)
            five_kind = val;
        if (dice_freq[i] == 6)
            six_kind = val;
    }

    // Pair (Highest)
    if (!pairs.empty())
        categories.push_back({Category::Pair, (uint8_t)(2 * pairs.back())});

    // Two Pair (Two Highest)
    if (pairs.size() >= 2)
        categories.push_back({Category::TwoPair, (uint8_t)(2 * pairs[pairs.size() - 1] + 2 * pairs[pairs.size() - 2])});

    // Three Pair
    if (pairs.size() >= 3)
        categories.push_back({Category::ThreePair, (uint8_t)(2 * (pairs[0] + pairs[1] + pairs[2]))});

    // Kinds
    if (!threes.empty())
        categories.push_back({Category::ThreeKind, (uint8_t)(3 * threes.back())});
    if (four_kind)
        categories.push_back({Category::FourKind, (uint8_t)(4 * four_kind)});
    if (five_kind)
        categories.push_back({Category::FiveKind, (uint8_t)(5 * five_kind)});

    // 3. Straight Logic (Corrected Indexing)
    int consecutive = 0, max_consecutive = 0, straight_start = -1, current_start = -1;
    for (int i = 0; i < 6; i++) {
        if (dice_freq[i] > 0) {
            if (consecutive == 0)
                current_start = i;
            consecutive++;
            if (consecutive > max_consecutive) {
                max_consecutive = consecutive;
                straight_start = current_start;
            }
        } else {
            consecutive = 0;
        }
    }

    // Small Straight (1-2-3-4-5) starts at index 0
    if (max_consecutive >= 5 && straight_start == 0)
        categories.push_back({Category::SmallStraight, 15});

    // Large Straight (2-3-4-5-6) starts at index 1 (or is part of a 6-long straight)
    if ((max_consecutive == 5 && straight_start == 1) || max_consecutive == 6)
        categories.push_back({Category::LargeStraight, 20});

    if (max_consecutive == 6) {
        categories.push_back({Category::Straight, 21});
    }

    // 4. Special Combinations (Sum of all dice)
    // House: 3 + 2 (In 6 dice, this means at least one 3-kind and two different pairs)
    if (threes.size() >= 1 && pairs.size() >= 2)
        categories.push_back({Category::House, (uint8_t)total_sum});

    // Villa: 3 + 3
    if (threes.size() >= 2)
        categories.push_back({Category::Villa, (uint8_t)total_sum});

    // Tower: 4 + 2
    if (four_kind && pairs.size() >= 2)
        categories.push_back({Category::Tower, (uint8_t)total_sum});

    // 5. Chance & Maxi Yahtzee
    categories.push_back({Category::Chance, (uint8_t)total_sum});

    if (six_kind)
        categories.push_back({Category::MaxiYahtzee, 100});

    // Store in lookup
    lookup.categories = categories;
    lookup.category_mask = 0;
    for (auto& entry : categories) {
        lookup.category_mask |= (1 << (int)entry.category);
    }

    // 1. Define your threshold
    const double HEURISTIC_THRESHOLD = 0.0; // Adjust as needed

    // 2. Remove "bad" entries first
    std::erase_if(lookup.categories, [HEURISTIC_THRESHOLD](const CategoryEntry& entry) {
        return get_score_heuristic(entry) <= HEURISTIC_THRESHOLD;
    });

    // 3. Sort the survivors
    std::sort(lookup.categories.begin(), lookup.categories.end(), [](const CategoryEntry& a, const CategoryEntry& b) {
        return get_score_heuristic(a) > get_score_heuristic(b);
    });
}

void init_index(std::array<uint8_t, 6>& dice_freq, DiceLookup& lookup) {
    int index = 0;
    int remaining_dice = 0;
    for (int f : dice_freq)
        remaining_dice += f; // total dice (e.g., 6)

    int n = remaining_dice;
    int k = 5; // faces - 1

    for (int i = 0; i < 5; ++i) {
        // Add combinations where this face had fewer dice than it actually does
        for (int j = 0; j < dice_freq[i]; ++j) {
            // If we put j dice here, how many ways to arrange the rest?
            // Remaining dice = n - j, remaining bins = k - 1
            // Stars and Bars: binom((n-j) + (k-1), (k-1))
            index += pascals[(n - j) + (k - 1)][k - 1];
        }
        n -= dice_freq[i];
        k--;
    }
    lookup.index = index;
}

void init_dice_lookups() {
    if (load_binary(CACHE_FILENAME, false)) {
        std::cout << "Loaded dice lookups from cache." << std::endl;
        return;
    }

    std::cout << "Cache not found. Computing lookup tables (this may take a while)..." << std::endl;

    for (int n = 0; n < 12; ++n) {
        pascals[n][0] = 1;
        for (int k = 1; k <= n; ++k) {
            pascals[n][k] = pascals[n - 1][k - 1] + pascals[n - 1][k];
        }
    }
    for (uint8_t a1 = 0; a1 <= 6; ++a1) {
        for (uint8_t a2 = 0; a2 <= 6 - a1; ++a2) {
            for (uint8_t a3 = 0; a3 <= 6 - a1 - a2; ++a3) {
                for (uint8_t a4 = 0; a4 <= 6 - a1 - a2 - a3; ++a4) {
                    for (uint8_t a5 = 0; a5 <= 6 - a1 - a2 - a3 - a4; ++a5) {
                        uint8_t a6 = 6 - a1 - a2 - a3 - a4 - a5;
                        std::array<uint8_t, 6> dice_freq = {a1, a2, a3, a4, a5, a6};
                        DiceLookup& lookup = dice_lookups[freq_to_index(dice_freq)];
                        init_categories(dice_freq, lookup);
                        init_ev(dice_freq, lookup);
                        init_index(dice_freq, lookup);
                        Dice dice = Dice(dice_freq);
                        all_dice[lookup.index] = dice;
                        std::cout << "i: " << lookup.index << " dice: " << dice.to_string() << std::endl;
                    }
                }
            }
        }
    }
    save_binary(CACHE_FILENAME, false);
}

void init_mask_lookups() {
    if (load_binary(MASK_CACHE_FILENAME, true)) {
        std::cout << "Loaded mask lookups from cache." << std::endl;
        return;
    }

    std::cout << "Computing mask lookups..." << std::endl;
#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < all_dice.size(); i++) {
        Dice dice = all_dice[i];
        std::cout << dice.to_string() << std::endl;
        for (int j = 1; j < mask_lookups.size(); j++) {
            mask_lookups[j].best_rerolls[i] = compute_best_reroll(dice, j);
        }
    }
    save_binary(MASK_CACHE_FILENAME, true);
}

static double factorial(int n) {
    static const double table[] = {1, 1, 2, 6, 24, 120, 720};
    return table[n];
}

static double get_hand_probability(const std::array<uint8_t, 6>& freq) {
    double combinations = factorial(6);
    for (int f : freq)
        combinations /= factorial(f);
    return combinations * std::pow(1.0 / 6.0, 6);
}

void calculate_global_category_evs() {
    std::array<double, (int)Category::Count> global_evs{0.0};
    std::array<double, (int)Category::Count> global_averages{0.0};
    std::array<uint32_t, (int)Category::Count> category_counts{0};

    double total_prob = 0.0;

    for (uint8_t a1 = 0; a1 <= 6; ++a1) {
        for (uint8_t a2 = 0; a2 <= 6 - a1; ++a2) {
            for (uint8_t a3 = 0; a3 <= 6 - a1 - a2; ++a3) {
                for (uint8_t a4 = 0; a4 <= 6 - a1 - a2 - a3; ++a4) {
                    for (uint8_t a5 = 0; a5 <= 6 - a1 - a2 - a3 - a4; ++a5) {
                        uint8_t a6 = 6 - a1 - a2 - a3 - a4 - a5;
                        Dice dice = Dice({a1, a2, a3, a4, a5, a6});

                        double p_hand = get_hand_probability(dice.dice_freq);
                        total_prob += p_hand;

                        DiceLookup lookup = get_dice_lookup(dice);

                        for (auto const& entry : lookup.categories) {
                            int idx = (int)entry.category;
                            global_evs[idx] += entry.score * p_hand;
                            global_averages[idx] += entry.score;
                            category_counts[idx] += 1;
                        }
                    }
                }
            }
        }
    }

    // Normalize averages
    for (int i = 0; i < (int)Category::Count; ++i) {
        if (category_counts[i] > 0) {
            global_averages[i] /= category_counts[i];
        }
    }

    std::cout << "--- Results ---\n";
    for (int i = 0; i < (int)Category::Count; ++i) {
        std::cout << category_to_string((Category)i) << " | EV: " << global_evs[i] << " | Avg Score: " << global_averages[i] << std::endl;
    }
}
