#pragma once

#include "types.h"

namespace PokerEye {
  struct GameConfig {
    uint8_t numPlayers = 6;

    Chips smallBlind = 50;
    Chips bigBlind = 100;

    Chips ante = 0;

    Chips startingStack = 10'000;
    uint64_t seed = 1;
  }
}
