#pragma once

#include "types.h"
#include "card.h"

#include <array>
#include <cstdint>

namespace Poker {
  class Deck {
  public:
    Deck() noexcept;

    void reset() noexcept;
    Card draw() noexcept;

    uint8_t remaining() const noexcept {
      return static_cast<uint8_t>(DECK_SIZE - next_);
    }

  private:
    std::array<card, deck_size> cards_;
    uint8_t next_ = 0;
  };
}
