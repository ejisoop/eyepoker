#pragma once

#include "hand_evaluator.h"
#include <array>

namespace PokerEye {
struct EquityRequest {
  GameRules rules = GameRules::holdem();
  uint8_t numPlayers = 2;
  std::array<std::array<Card, MAX_HOLE_CARDS>, MAX_PLAYERS> holeCards{};
  std::array<Card, 5> board{};
  uint8_t boardCount = 0;
  uint64_t deadCards = 0; // Bit i marks card i unavailable.
  uint64_t trials = 100'000;
  uint64_t seed = 42;
  unsigned threads = 1;

  EquityRequest() noexcept {
    for (auto& h : holeCards) h.fill(INVALID_CARD);
    board.fill(INVALID_CARD);
  }
};
struct PlayerEquity {
  uint64_t wins = 0; // Outright wins.
  uint64_t ties = 0; // Trials sharing first place.
  double equity = 0; // Mean fraction of an equally contested pot.
  double standardError = 0; // Sample SE of fractional shares; NaN for one trial.
};
struct EquityResult {
  uint64_t trials = 0;
  uint8_t numPlayers = 0;
  std::array<PlayerEquity, MAX_PLAYERS> players{};
};

// Samples uniformly from all legal completions. No range weighting or side pots.
// Validates once, then uses allocation-free trial loops. The same seed/request
// gives identical results regardless of worker count or scheduling.
EquityResult simulateEquity(const EquityRequest& request);
}
