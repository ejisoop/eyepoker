#pragma once

#include "card.h"
#include <stdexcept>

namespace PokerEye {
enum class HandSelection : uint8_t { AnyFive, ExactlyTwo };
enum class BettingStructure : uint8_t { NoLimit, PotLimit };

// Community-card high poker. Change rules without changing the betting engine.
// The board always has five cards. Short Deck house rules vary: these defaults
// use A-6-7-8-9 as the low straight and flush > full house > straight > trips.
struct GameRules {
  uint8_t holeCards = 2;
  uint8_t minimumRank = 0;
  HandSelection selection = HandSelection::AnyFive;
  BettingStructure betting = BettingStructure::NoLimit;
  bool flushBeatsFullHouse = false;
  bool tripsBeatStraight = false;
  bool burnCards = true;

  static constexpr GameRules holdem() noexcept { return {}; }
  static constexpr GameRules omaha(uint8_t cards = 4) noexcept {
    GameRules r;
    r.holeCards = cards;
    r.selection = HandSelection::ExactlyTwo;
    r.betting = BettingStructure::PotLimit;
    return r;
  }
  static constexpr GameRules shortDeck() noexcept {
    GameRules r;
    r.minimumRank = 4;
    r.flushBeatsFullHouse = true;
    return r;
  }
  void validate() const {
    if (holeCards < 2 || holeCards > MAX_HOLE_CARDS || minimumRank > 8 ||
        (selection != HandSelection::AnyFive && selection != HandSelection::ExactlyTwo) ||
        (betting != BettingStructure::NoLimit && betting != BettingStructure::PotLimit))
      throw std::invalid_argument("invalid community-card rules");
  }
};
}
