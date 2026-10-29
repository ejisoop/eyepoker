#pragma once

#include "types.h"

#include <array>
#include <cstdint>

namespace Poker {
  using HandRank = uint32_t;

  class HandEvaluator {
  public:
    static HandRank evaluateSeven(const std::array<Card, 7>& cards) noexcept;
    static HandRank evaluateFive(const std::array<Card, 5>& cards) noexcept;
  };
}
