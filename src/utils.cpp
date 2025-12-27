#include "utils.h"
#include <cstdlib>
#include <stdexcept>

thread_local uint32_t rng_state = 321321321;

uint32_t xorshift32() {
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

uint32_t fast_rand(uint32_t max) {
    return ((uint64_t)xorshift32() * max) >> 32;
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
