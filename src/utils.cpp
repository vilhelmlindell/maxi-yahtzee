#include "utils.h"
#include "game.h"
#include "lookup.h"
#include <cstdlib>
#include <stdexcept>

thread_local uint32_t rng_state = 167;

//static uint8_t rng(void) {
//    return rng_state++ * 29;
//}
static uint8_t rng(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return (uint8_t)(rng_state & 0xFF);
}

uint8_t fast_rand(uint8_t max) {
    //return rand() % max;
    return ((uint16_t)rng() * max) >> 8;
}

uint8_t random_set_bit_u32(uint32_t x) {
    uint32_t start = rng() & 31;
    uint32_t y = (x >> start) | (x << (32 - start));
    return ((__builtin_ctz(y) + start) & 31);
}
uint8_t random_set_bit_u64(uint64_t x) {
    uint64_t start = rng() & 63;
    uint64_t y = (x >> start) | (x << (64 - start));
    return ((__builtin_ctzll(y) + start) & 63);
}

uint32_t parse_uint(const std::string& s) {
    size_t idx = 0;
    uint32_t value = std::stoul(s, &idx, 0);
    if (idx != s.size())
        throw std::invalid_argument("Invalid integer: " + s);
    return value;
}

std::optional<uint8_t> next_valid_category(DiceLookup& lookup, uint32_t mask, uint8_t start) {
    // Iterate through the categories vector starting from 'start'
    for (uint8_t i = start; i < lookup.categories.size(); ++i) {
        // Get the specific category (e.g., Category::Ones)
        int category_id = (int)lookup.categories[i].category;

        // Check if this bit is NOT set in the mask
        // mask represents already scored categories
        if (mask & (1 << category_id)) {
            return i; // Found the next available category index
        }
    }

    // Return a sentinel value (like the size of the vector) 
    // to indicate no valid category was found.
    return std::nullopt;
}

Category worst_category(uint32_t mask) {
    Category worst;
    double worst_score = 1000000000.0;
    for (int i = 0; i < 20; i++) {
        if (mask & (1 << i)) {
            double score = expected_values[i];
            if (score < worst_score) {
                worst_score = score;
                worst = (Category)i;
            }
        }
    }
    return worst;
}
