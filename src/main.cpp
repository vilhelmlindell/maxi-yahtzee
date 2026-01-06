#include "game.h"
#include "mcts.h"
#include "lookup.h"
#include "run.h"
#include "utils.h"
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>

extern thread_local uint32_t rng_state;

void test_distribution(uint8_t max, uint32_t iterations) {
    uint32_t counts[256] = {0}; // Buckets for every possible uint8_t output
    
    // 1. Run the generator
    for (uint32_t i = 0; i < iterations; i++) {
        counts[fast_rand(max)]++;
    }

    // 2. Print results
    printf("Distribution Test (max=%d, iterations=%u):\n", max, iterations);
    uint32_t expected = iterations / max;
    
    for (int i = 0; i < max; i++) {
        float deviation = ((float)counts[i] - expected) / expected * 100.0f;
        printf("[%3d]: %6u counts (%+.2f%% deviation)\n", i, counts[i], deviation);
    }
}

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(nullptr)));
    rng_state = rand();
    std::random_device rd;
    std::mt19937 gen(rd());

    init_dice_lookups();
    init_mask_lookups();

    //test_distribution(64, 10000000);

    //Dice dice = Dice({0, 1, 1, 2, 1, 1});
    //DiceLookup lookup = get_dice_lookup(dice);
    //for (auto& reroll : lookup.sorted_rerolls) {
    //    std::cout << reroll.to_string() << std::endl;
    //}
    //uint32_t mask = 0b1111111111;

    run_args(argc, argv);

    //Dice dice = Dice({1, 1, 0, 4, 0, 0});
    //DiceLookup lookup = get_dice_lookup(dice);
    //for (int i = 0; i < lookup.sorted_rerolls.size(); i++) {
    //    std::cout << lookup.sorted_rerolls[i].to_string() << std::endl;
    //    for (int j = 0; j < 20; j++) {
    //        std::cout << category_to_string((Category)j) << " " << (float)lookup.reroll_category_evs[i][j] << std::endl;
    //    }
    //}
}
