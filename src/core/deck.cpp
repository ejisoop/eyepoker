#include <utility>
#include <cassert>

#include <pokereye/core/deck.h>

using namespace PokerEye;

template <ShuffleStrategy Strategy>
Deck<Strategy>::Deck() noexcept {
  restore(); }
template <ShuffleStrategy Strategy>
void Deck<Strategy>::restore() noexcept {
  for (uint8_t i = 0; i < DECK_SIZE; i++) {
    cards_[i] = static_cast<Card>(i);
  }

  next_ = 0;
}

template <ShuffleStrategy Strategy>
void Deck<Strategy>::reset(Random& rng) noexcept {
  next_ = 0;

  if constexpr (Strategy == ShuffleStrategy::Full) {
    for (uint8_t i = 0; i < DECK_SIZE - 1; i++) {
      const uint8_t remaining = DECK_SIZE - i;
      const uint8_t randIdx = static_cast<uint8_t>(rng.bounded(remaining) + i);
      std::swap(cards_[i], cards_[randIdx]);
    }
  }
}

template <ShuffleStrategy Strategy>
Card Deck<Strategy>::draw(Random& rng) noexcept {
  assert(next_ < DECK_SIZE);

  if constexpr (Strategy == ShuffleStrategy::Lazy) {
    const uint8_t remaining = DECK_SIZE - next_;
    const uint8_t randIdx = static_cast<uint8_t>(rng.bounded(remaining) + next_);
    std::swap(cards_[next_], cards_[randIdx]);
  }

  return cards_[next_++];
}

template <ShuffleStrategy Strategy>
uint8_t Deck<Strategy>::remaining() const noexcept { 
  return static_cast<uint8_t>(DECK_SIZE - next_); 
} 

template class Deck<ShuffleStrategy::Lazy>;
template class Deck<ShuffleStrategy::Full>;
