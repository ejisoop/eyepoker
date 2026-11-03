#include <utility>
#include <cassert>
#include <stdexcept>

#include <pokereye/core/deck.h>

using namespace PokerEye;

template <ShuffleStrategy Strategy>
Deck<Strategy>::Deck() noexcept {
  restore();
}
template <ShuffleStrategy Strategy>
Deck<Strategy>::Deck(uint8_t minimumRank) : minimumRank_(minimumRank) {
  if (minimumRank > 8) throw std::invalid_argument("minimum deck rank must be 0..8");
  size_ = static_cast<uint8_t>(DECK_SIZE - minimumRank * 4);
  restore();
}
template <ShuffleStrategy Strategy>
void Deck<Strategy>::restore() noexcept {
  for (uint8_t i = 0; i < size_; i++) {
    cards_[i] = static_cast<Card>(i + minimumRank_ * 4);
  }

  next_ = 0;
}

template <ShuffleStrategy Strategy>
void Deck<Strategy>::reset(Random& rng) noexcept {
  next_ = 0;

  if constexpr (Strategy == ShuffleStrategy::Full) {
    for (uint8_t i = 0; i < size_ - 1; i++) {
      const uint8_t remaining = size_ - i;
      const uint8_t randIdx = static_cast<uint8_t>(rng.bounded32(remaining) + i);
      std::swap(cards_[i], cards_[randIdx]);
    }
  }
}

template <ShuffleStrategy Strategy>
Card Deck<Strategy>::draw(Random& rng) noexcept {
  assert(next_ < size_);

  if constexpr (Strategy == ShuffleStrategy::Lazy) {
    const uint8_t remaining = size_ - next_;
    const uint8_t randIdx = static_cast<uint8_t>(rng.bounded32(remaining) + next_);
    std::swap(cards_[next_], cards_[randIdx]);
  }

  return cards_[next_++];
}

template <ShuffleStrategy Strategy>
uint8_t Deck<Strategy>::remaining() const noexcept { 
  return static_cast<uint8_t>(size_ - next_);
} 

template class PokerEye::Deck<ShuffleStrategy::Lazy>;
template class PokerEye::Deck<ShuffleStrategy::Full>;
