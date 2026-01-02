#ifndef UTILS_HPP
#define UTILS_HPP

#include "lookup.h"
#include <cstdint>
#include <optional>
#include <string>

uint8_t fast_rand(uint8_t max);
uint8_t random_set_bit_u32(uint32_t x);
uint8_t random_set_bit_u64(uint64_t x);
uint32_t parse_uint(const std::string& s);

std::optional<uint8_t> next_valid_category(DiceLookup& lookup, uint32_t mask, uint8_t start);
Category worst_category(uint32_t mask);

#endif // UTILS_HPP
