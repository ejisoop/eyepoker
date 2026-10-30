#pragma once

#include <cstdint>
#include <array>

/*
 * using xoshiro256++
 */

namespace PokerEye {
  class Random {
  public:
    Random(uint64_t seed) noexcept {
      reseed(seed);
    }

    void reseed(uint64_t seed) noexcept;
    uint64_t next() noexcept;
    uint64_t bounded(uint64_t bound) noexcept;

    const std::array<uint64_t, 4>& state() const noexcept {
      return state_;
    }

  private:
    std::array<uint64_t, 4> state_{};
    
    static uint64_t splitmix(uint64_t& seed) noexcept;
  };
}
