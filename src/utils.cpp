#include "utils.h"
#include <cstdlib>
#include <stdexcept>

thread_local uint8_t rng_state = 167;

static uint8_t rng(void) {
    return rng_state++ * 29;
}

uint8_t fast_rand(uint8_t max) {
    return ((uint16_t)rng() * max) >> 8;
}

uint32_t random_set_bit(uint32_t x) {
    uint32_t start = rand() & 31;
    uint32_t y = (x >> start) | (x << (32 - start));
    return ((__builtin_ctz(y) + start) & 31);
}

uint32_t parse_uint(const std::string& s) {
    size_t idx = 0;
    uint32_t value = std::stoul(s, &idx, 0); // base=0 → auto-detect
    if (idx != s.size())
        throw std::invalid_argument("Invalid integer: " + s);
    return value;
}
