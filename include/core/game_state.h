#pragma once

#include "types.h"

#include <array>
#include <cstdint>

namespace Poker {
  struct PlayerState {
    std::array<Card, 2> holeCards{
      INVALID_CARD, INVALID_CARD
    };

    Chips stack = 0;

    Chips streetBet = 0;
    Chips totalBet = 0;

    PlayerStatus status = PlayerStatus::None;

    bool needsAction = false;
  };

  struct GameState {
    std::array<PlayerState, MAX_PLAYERS> players;

    std::array<Card, 5> board{
      INVALID_CARD,
      INVALID_CARD,
      INVALID_CARD,
      INVALID_CARD,
      INVALID_CARD
    };

    uint8_t boardCount = 0;

    Street street = Street::Preflop;
    HandStatus status = HandStatus::Waiting;

    PlayerId dealerButton = INVALID_PLAYER;
    PlayerId currentPlayer = INVALID_PLAYER;

    uint8_t numPlayers = 0;

    Chips pot = 0;
    Chips currentBet = 0;

    Chips lastFullRaise = 0;
    uint64_t handNumber = 0;
  };
}
