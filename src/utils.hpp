#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdint>
#include <string>

uint32_t xorshift32();
uint32_t fast_rand(uint32_t max);
uint32_t random_set_bit(uint32_t x);
uint32_t parse_uint(const std::string& s);

#endif // UTILS_HPP
