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

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(nullptr)));
    rng_state = rand();
    std::random_device rd;
    std::mt19937 gen(rd());

    init_dice_lookups();
    init_mask_lookups();

    //Dice dice = Dice({0, 1, 1, 2, 1, 1});
    //DiceLookup lookup = get_dice_lookup(dice);
    //for (int i = 0; i < 63; i++) {
    //    std::cout << lookup.rerolls[i].to_string() << std::endl;
    //}
    //uint32_t mask = 0b1111111111;
    //std::cout << get_best_reroll(dice, mask).to_string() << std::endl;
    //std::cout << get_best_reroll(dice, mask).to_string() << std::endl;
    //std::cout << get_best_reroll(dice, mask).to_string() << std::endl;
    //std::cout << get_best_reroll(dice, mask).to_string() << std::endl;

    run_args(argc, argv);

    //Dice dice = Dice({1, 1, 4, 0, 0, 0});
    //DiceLookup lookup = get_dice_lookup(dice);
    //for (int i = 0; i < 63; i++) {
    //    std::cout << lookup.sorted_rerolls[i].to_string() << std::endl;
    //    for (int j = 0; j < 20; j++) {
    //        std::cout << category_to_string((Category)j) << " " << (float)lookup.reroll_category_evs[i][j] << std::endl;
    //    }
    //}
}
