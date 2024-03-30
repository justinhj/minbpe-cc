// Required to instantiate the lambda
#include "Tokenizer.h"

std::function<std::size_t(const merge_key_t&)> key_hash_lambda = 
    [](const merge_key_t& k) -> std::size_t {
      std::size_t seed = 0;
      std::hash<int> hasher;
      seed ^= hasher(std::get<0>(k)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      seed ^= hasher(std::get<1>(k)) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      return seed;
};
