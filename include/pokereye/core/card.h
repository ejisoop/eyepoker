#pragma once

#include "types.h"

namespace PokerEye {
  /*
   * card = rank * 4 + suit; ranks 0..12 represent deuce..ace.
   */

  constexpr uint8_t RANK_COUNT = 13;
  constexpr uint8_t SUIT_COUNT = 4;
  constexpr uint8_t DECK_SIZE = RANK_COUNT * SUIT_COUNT;

  constexpr uint8_t rank(Card c) noexcept {
    return c >> 2;
  }

  constexpr uint8_t suit(Card c) noexcept {
    return c & 3;
  }

  constexpr Card newCard(uint8_t rank, uint8_t suit) noexcept {
    return static_cast<Card>(rank * SUIT_COUNT + suit);
  }

  constexpr bool validateCard(Card c) noexcept {
    return c < DECK_SIZE;
  }
}
