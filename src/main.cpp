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

int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(nullptr)));
    rng_state = rand();
    std::random_device rd;
    std::mt19937 gen(rd());

    init_dice_lookups();
    init_mask_lookups();
    run_args(argc, argv);
}
