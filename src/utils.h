#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdint>
#include <string>

uint8_t fast_rand(uint8_t max);
uint32_t random_set_bit_u32(uint32_t x);
uint64_t random_set_bit_u64(uint64_t x);
uint32_t parse_uint(const std::string& s);

#endif // UTILS_HPP
