#pragma once

#include "action.h"
#include <array>

namespace PokerEye {
// Policy view: no opponent hole cards, deck order, or RNG state.
struct Observation {
  uint64_t handNumber = 0;
  PlayerId hero = INVALID_PLAYER;
  PlayerId button = INVALID_PLAYER;
  Street street = Street::Preflop;
  std::array<Card, MAX_HOLE_CARDS> holeCards{
      INVALID_CARD, INVALID_CARD, INVALID_CARD,
      INVALID_CARD, INVALID_CARD, INVALID_CARD};
  uint8_t holeCardCount = 0;
  std::array<Card, 5> board{
      INVALID_CARD, INVALID_CARD, INVALID_CARD, INVALID_CARD, INVALID_CARD};
  uint8_t boardCount = 0;
  uint8_t numPlayers = 0;
  std::array<Chips, MAX_PLAYERS> stacks{};
  std::array<Chips, MAX_PLAYERS> streetBets{};
  std::array<Chips, MAX_PLAYERS> totalBets{};
  uint16_t activeMask = 0; // Players still eligible, including all-ins.
  uint16_t allInMask = 0;
  Chips pot = 0;
  LegalActions legal{};
};
}
