#include <pokereye/core/hand_evaluator.h>
#include <pokereye/core/card.h>

#include <bit>

namespace {
using namespace PokerEye;
using Mask = uint32_t;

inline unsigned highest(Mask mask) noexcept {
  return 31u - std::countl_zero(mask);
}

inline HandRank topRanks(Mask mask, unsigned count) noexcept {
  HandRank result = 0;
  for (unsigned shift = 16; count != 0; --count, shift -= 4) {
    const unsigned r = highest(mask);
    result |= r << shift;
    mask &= ~(1u << r);
  }
  return result;
}

inline HandRank score(unsigned category, unsigned first, unsigned second = 0) noexcept {
  return (category << 20) | (first << 16) | (second << 12);
}

// consecutive runs end at their highest bit. ace also plays below deuce.
inline int straightHigh(Mask mask) noexcept {
  const Mask runs = mask & (mask << 1) & (mask << 2) & (mask << 3) & (mask << 4);
  if (runs) return static_cast<int>(highest(runs));
  return (mask & 0x100fu) == 0x100fu ? 3 : -1;
}

template <std::size_t N>
inline HandRank evaluate(const std::array<Card, N>& cards) noexcept {
  Mask suits[4]{};
  Mask ranks = 0, pairs = 0, trips = 0, quads = 0;
  for (Card c : cards) {
    const Mask bit = 1u << rank(c);
    // update from highest multiplicity downward, before inserting this card.
    quads |= trips & bit;
    trips |= pairs & bit;
    pairs |= ranks & bit;
    ranks |= bit;
    suits[suit(c)] |= bit;
  }

  // with at most seven cards, a flush cannot coexist with a full house/quads.
  for (Mask suited : suits) {
    if (std::popcount(suited) >= 5) {
      const int straight = straightHigh(suited);
      if (straight >= 0) return score(8, straight);
      return (5u << 20) | topRanks(suited, 5);
    }
  }
  if (quads) {
    const unsigned r = highest(quads);
    return score(7, r, highest(ranks & ~(1u << r)));
  }
  if (trips) {
    const unsigned r = highest(trips);
    const Mask others = pairs & ~(1u << r);
    if (others) return score(6, r, highest(others));
  }
  const int straight = straightHigh(ranks);
  if (straight >= 0) return score(4, straight);
  if (trips) {
    const unsigned r = highest(trips);
    return score(3, r) | (topRanks(ranks & ~(1u << r), 2) >> 4);
  }
  if (pairs) {
    const unsigned first = highest(pairs);
    const Mask rest = pairs & ~(1u << first);
    if (rest) {
      const unsigned second = highest(rest);
      const Mask kickers = ranks & ~((1u << first) | (1u << second));
      return score(2, first, second) | (highest(kickers) << 8);
    }
    return score(1, first) | (topRanks(ranks & ~(1u << first), 3) >> 4);
  }
  return topRanks(ranks, 5);
}
}

PokerEye::HandRank PokerEye::HandEvaluator::evaluateFive(const std::array<Card, 5>& cards) noexcept {
  return evaluate(cards);
}

PokerEye::HandRank PokerEye::HandEvaluator::evaluateSeven(const std::array<Card, 7>& cards) noexcept {
  return evaluate(cards);
}
