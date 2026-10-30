#pragma once

#include <cstdint>
#include <limits>

namespace PokerEye {
  using Chips = int64_t;
  using PlayerId = uint8_t;
  using Card = uint8_t;

  constexpr PlayerId MAX_PLAYERS = 12; // arbitrary max
  constexpr PlayerId INVALID_PLAYER = 255;
  constexpr Card INVALID_CARD = 255;

  enum class Street : uint8_t {
    Preflop,
    Flop,
    Turn,
    River,
    Showdown
  };

  enum class PlayerStatus : uint8_t {
    None,
    Active,
    Folded,
    AllIn,
    SittingOut
  };

  enum class HandStatus : uint8_t {
    Waiting,
    InProgress,
    Complete
  };
}
