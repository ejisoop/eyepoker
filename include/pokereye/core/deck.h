#pragma once

#include <pokereye/core/types.h>
#include <pokereye/core/card.h>
#include <pokereye/core/random.h>

#include <array>
#include <cstdint>

namespace PokerEye {
  enum class ShuffleStrategy : uint8_t {
    Lazy,
    Full
  };

  template<ShuffleStrategy Strategy = ShuffleStrategy::Lazy>
  class Deck {
  public:
    Deck() noexcept;

    void restore() noexcept;

    Card draw(Random& rng) noexcept;
    void reset(Random& rng) noexcept;

    uint8_t remaining() const noexcept;

  private:
    std::array<Card, DECK_SIZE> cards_{};
    uint8_t next_ = 0;
  };

  extern template class Deck<ShuffleStrategy::Lazy>;
  extern template class Deck<ShuffleStrategy::Full>;
}
