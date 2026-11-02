#pragma once

#include "types.h"

#include <array>
#include <cstdint>

namespace PokerEye {
  using HandRank = uint32_t;

  // larger scores win. bits 20..23 hold the category (0 high card through
  // 8 straight flush), five descending tie-break ranks occupy 4-bit fields.
  // rank fields use 0..12 for deuce..ace, unused fields are zero.

  class HandEvaluator {
  public:
    [[nodiscard]]
    static HandRank evaluateFive(const std::array<Card, 5>& cards) noexcept;

    [[nodiscard]]
    static HandRank evaluateSeven(const std::array<Card, 7>& cards) noexcept;
  };
}
