#pragma once

#include "types.h"

namespace Poker {
  /*
   * card = suit * 4 + rank
   */

  constexpr uint8_t DECK_SIZE = 52;

  constexpr uint8_t rank(Card c) noexcept {
    return c & 3;
  }
  
  constexpr uint8_t suit(Card c) noexcept {
    return c >> 2;
  }

  constexpr Card newCard(uint8_t rank, uint8_t suit) noexcept {
    return static_cast<Card>(suit * 4 + rank);
  }

  constexpr bool validateCard(Card c) noexcept {
    return c < DECK_SIZE;
  }
}
