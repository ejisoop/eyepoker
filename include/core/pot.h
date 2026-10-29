#pragma once

#include "game_state.h"
#include "hand_evaluator.h"

#include <array>

namespace Poker {
  struct PotResult {
    std::array<Chips, MAX_PLAYERS> winnings{};
  }

  class PotManager {
  public:
    static PotResult settle(
        const GameState& state, 
        const std::array<HandRank, MAX_PLAYERS>& ranks
    ) noexcept;
  };
}
