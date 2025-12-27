#include "utils.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include "run.h"

extern thread_local uint32_t rng_state;

int main() {
    srand(static_cast<unsigned>(time(nullptr)));
    rng_state = rand();

    run_interface();
}
