#pragma once

#include "types.h"
#include "rules.h"
#include <array>

namespace PokerEye {
  struct GameConfig {
    uint8_t numPlayers = 6;

    Chips smallBlind = 50;
    Chips bigBlind = 100;

    Chips ante = 0; // Individual ante, paid before blinds.

    Chips startingStack = 10'000;
    uint64_t seed = 42;

    ShuffleStrategy shuffleStrategy = ShuffleStrategy::Lazy;
    GameRules rules = GameRules::holdem();
    // All zeros selects startingStack. Otherwise these are exact seat stacks;
    // zero-stack seats sit out. Allows unequal stacks and side-pot simulations.
    std::array<Chips, MAX_PLAYERS> initialStacks{};
  };
}
