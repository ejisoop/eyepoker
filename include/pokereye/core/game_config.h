#pragma once

#include "types.h"

namespace PokerEye {
  struct GameConfig {
    uint8_t numPlayers = 6;

    Chips smallBlind = 50;
    Chips bigBlind = 100;

    Chips ante = 0; // not planned for now...?

    Chips startingStack = 10'000;
    uint64_t seed = 42;

    ShuffleStrategy shuffleStrategy = ShuffleStrategy::Full;
  }
}
