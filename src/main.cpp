#include "utils.hpp"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include "run.hpp"

extern thread_local uint32_t rng_state;

void distribution() {
    uint32_t mask = 0b10000000100000001000000010000000;
    std::array<int, 32> freq_arr{};
    for (int i = 0; i < 10000; i++) {
        uint32_t j = random_set_bit(mask);
        freq_arr[j]++;
    }
    for (int i = 0; i < 32; i++) {
        std::cout << "i: " << i << " freq: " << freq_arr[i] << std::endl;
    }
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));
    rng_state = rand();

    run_games();
}
