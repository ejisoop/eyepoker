#pragma once

#include "action.h"
#include "types.h"

#include <array>

namespace PokerEye {
  struct Observation {
    uint64_t handNumber;
    PlayerId hero;
    PlayerId button;

    Street street;

    std::array<Card, 2> holeCards;
    std::array<Card, 5> board;
    uint8_t boardCount;

    std::array<Chips, MAX_PLAYERS> stacks;
    std::array<Chips, MAX_PLAYERS> streetBets;

    uint16_t activeMask;

    Chips pot;

    LegalActions legal;
  };
}
