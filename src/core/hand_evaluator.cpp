#include <pokereye/core/hand_evaluator.h>
#include <pokereye/core/card.h>

#include <bit>
#include <algorithm>

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
inline int straightHigh(Mask mask, unsigned minimumRank = 0) noexcept {
  const Mask runs = mask & (mask << 1) & (mask << 2) & (mask << 3) & (mask << 4);
  if (runs) return static_cast<int>(highest(runs));
  const Mask wheel = 0x1000u | (0xfu << minimumRank);
  return (mask & wheel) == wheel ? static_cast<int>(minimumRank + 3) : -1;
}

template <std::size_t N, bool Custom = false>
inline HandRank evaluate(const std::array<Card, N>& cards, const GameRules& rules = {}) noexcept {
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
      const int straight = straightHigh(suited, Custom ? rules.minimumRank : 0);
      if (straight >= 0) return score(8, straight);
      return ((Custom && rules.flushBeatsFullHouse ? 6u : 5u) << 20) | topRanks(suited, 5);
    }
  }
  if (quads) {
    const unsigned r = highest(quads);
    return score(7, r, highest(ranks & ~(1u << r)));
  }
  if (trips) {
    const unsigned r = highest(trips);
    const Mask others = pairs & ~(1u << r);
    if (others) return score(Custom && rules.flushBeatsFullHouse ? 5 : 6, r, highest(others));
  }
  const int straight = straightHigh(ranks, Custom ? rules.minimumRank : 0);
  if (straight >= 0 && !(Custom && rules.tripsBeatStraight && trips))
    return score(Custom && rules.tripsBeatStraight ? 3 : 4, straight);
  if (trips) {
    const unsigned r = highest(trips);
    return score(Custom && rules.tripsBeatStraight ? 4 : 3, r) | (topRanks(ranks & ~(1u << r), 2) >> 4);
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

PokerEye::HandRank PokerEye::HandEvaluator::evaluateFive(
    const std::array<Card, 5>& cards, const GameRules& rules) noexcept {
  return evaluate<5, true>(cards, rules);
}

PokerEye::HandRank PokerEye::HandEvaluator::evaluateShowdown(
    const std::array<Card, MAX_HOLE_CARDS>& hole, const std::array<Card, 5>& board,
    const GameRules& rules) noexcept {
  if (rules.selection == HandSelection::AnyFive && rules.holeCards == 2) {
    const std::array<Card, 7> cards{hole[0], hole[1], board[0], board[1], board[2], board[3], board[4]};
    if (rules.minimumRank == 0 && !rules.flushBeatsFullHouse && !rules.tripsBeatStraight)
      return evaluate(cards);
    return evaluate<7, true>(cards, rules);
  }
  HandRank best = 0;
  if (rules.selection == HandSelection::ExactlyTwo) {
    // Omaha: exactly two hole cards AND exactly three board cards.
    for (unsigned a = 0; a + 1 < rules.holeCards; ++a)
      for (unsigned b = a + 1; b < rules.holeCards; ++b)
        for (unsigned c = 0; c < 3; ++c)
          for (unsigned d = c + 1; d < 4; ++d)
            for (unsigned e = d + 1; e < 5; ++e)
              best = std::max(best, evaluate<5, true>(
                  std::array<Card, 5>{hole[a], hole[b], board[c], board[d], board[e]}, rules));
  } else {
    // Flexible larger-hole-card variants take a correctness-first fallback.
    std::array<Card, MAX_HOLE_CARDS + 5> cards{};
    std::copy_n(hole.begin(), rules.holeCards, cards.begin());
    std::copy(board.begin(), board.end(), cards.begin() + rules.holeCards);
    const unsigned n = rules.holeCards + 5;
    for (unsigned a = 0; a + 4 < n; ++a)
      for (unsigned b = a + 1; b + 3 < n; ++b)
        for (unsigned c = b + 1; c + 2 < n; ++c)
          for (unsigned d = c + 1; d + 1 < n; ++d)
            for (unsigned e = d + 1; e < n; ++e)
              best = std::max(best, evaluate<5, true>(
                  std::array<Card, 5>{cards[a], cards[b], cards[c], cards[d], cards[e]}, rules));
  }
  return best;
}
