#include <core/random.h>

#include <cassert>
#include <bit>

using namespace Poker;

void Random::reseed(uint64_t seed) noexcept {
  for (auto& word : state_) {
    word = splitmix(seed);
  }
}

uint64_t Random::next() noexcept {
  const uint64_t result = std::rotl(state_[0] + state_[3], 23) + state_[0];
  const uint64_t t = state_[1] << 17;

  state_[2] ^= state_[0];
  state_[3] ^= state_[1];
  state_[1] ^= state_[2];
  state_[0] ^= state_[3];

  state_[2] ^= t;
  state_[3] = std::rotl(state_[3], 45);

  return result;
}

uint64_t Random::bounded(uint64_t bound) noexcept {
  assert(bound > 0);

  const uint64_t threshold = -bound % bound;

  while (true) {
    uint64_t value = next();

    if (value >= threshold) {
      return value % bound;
    }
  }
}

static uint64_t splitmix(uint64_t& seed) noexcept {
  uint64_t z = (seed += 0x9E3779B97F4A7C15ULL);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}
