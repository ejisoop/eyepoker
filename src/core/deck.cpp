#include <random>
#include <algorithm>

#include <core/deck.h>

using namespace Poker;

Deck::Deck() noexcept {
  
}

void Deck::reset() noexcept {
  next_ = 0;
  
  for (uint8_t i = 0; i < DECK_SIZE - 1; i++) {

  }
}

Card draw() noexcept {
  return cards_[next_++];
}
