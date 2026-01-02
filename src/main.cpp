#include "game.h"
#include "mcts.h"
#include "lookup.h"
#include "run.h"
#include "utils.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>

extern thread_local uint32_t rng_state;

int main() {
    srand(static_cast<unsigned>(time(nullptr)));
    rng_state = rand();
    std::random_device rd;
    std::mt19937 gen(rd());

    init_dice_lookups();
    init_mask_lookups();
    //for (int i = 0; i < 1000; i++) {
    //    Dice dice = Dice();
    //    std::cout << dice.to_string() << std::endl;
    //    DiceLookup lookup = get_dice_lookup(dice);
    //    for (auto& entry : lookup.categories) {
    //        std::cout << category_to_string(entry.category) << " " << (int)entry.score << ": " << get_score_heuristic(entry) << std::endl;
    //    }
    //    std::cout << std::endl;
    //}
    //calculate_global_category_evs();
    run_interface();
}

