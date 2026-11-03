#pragma once
#include <pokereye/core/hand_evaluator.h>
#include <algorithm>
#include <numeric>

// Deliberately slow independent count-and-sort oracle; never used by the core.
inline PokerEye::HandRank referenceFive(const std::array<PokerEye::Card, 5>& cards,
                                        const PokerEye::GameRules& rules = {}) {
  using namespace PokerEye;
  unsigned counts[13]{};
  bool flush = true;
  for (Card c : cards) { ++counts[rank(c)]; flush &= suit(c) == suit(cards[0]); }
  std::array<unsigned, 13> ranks;
  std::iota(ranks.begin(), ranks.end(), 0);
  std::sort(ranks.begin(), ranks.end(), [&](unsigned a, unsigned b) {
    return counts[a] != counts[b] ? counts[a] > counts[b] : a > b;
  });
  int straight = -1;
  for (int high = 12; high >= 4; --high) {
    bool run = true;
    for (int i = 0; i < 5; ++i) run &= counts[high - i] != 0;
    if (run) { straight = high; break; }
  }
  if (straight < 0 && counts[12]) {
    bool low = true;
    for (unsigned i = 0; i < 4; ++i) low &= counts[rules.minimumRank + i] != 0;
    if (low) straight = rules.minimumRank + 3;
  }
  unsigned category = 0;
  std::array<unsigned, 5> tie{};
  if (straight >= 0 && flush) { category = 8; tie[0] = static_cast<unsigned>(straight); }
  else if (counts[ranks[0]] == 4) { category = 7; tie[0] = ranks[0]; tie[1] = ranks[1]; }
  else if (counts[ranks[0]] == 3 && counts[ranks[1]] == 2) {
    category = rules.flushBeatsFullHouse ? 5 : 6; tie[0] = ranks[0]; tie[1] = ranks[1];
  } else if (flush) { category = rules.flushBeatsFullHouse ? 6 : 5; std::copy_n(ranks.begin(), 5, tie.begin()); }
  else if (straight >= 0) { category = rules.tripsBeatStraight ? 3 : 4; tie[0] = static_cast<unsigned>(straight); }
  else {
    category = counts[ranks[0]] == 3 ? (rules.tripsBeatStraight ? 4 : 3) :
               counts[ranks[0]] == 2 ? (counts[ranks[1]] == 2 ? 2 : 1) : 0;
    const unsigned n = counts[ranks[0]] == 3 ? 3 : category == 2 ? 3 : category == 1 ? 4 : 5;
    std::copy_n(ranks.begin(), n, tie.begin());
  }
  HandRank result = category << 20;
  for (unsigned i = 0; i < 5; ++i) result |= tie[i] << (16 - 4 * i);
  return result;
}

inline PokerEye::HandRank referenceBest(const PokerEye::Card* cards, unsigned n,
                                       const PokerEye::GameRules& rules = {}) {
  PokerEye::HandRank best = 0;
  for (unsigned a = 0; a + 4 < n; ++a)
    for (unsigned b = a + 1; b + 3 < n; ++b)
      for (unsigned c = b + 1; c + 2 < n; ++c)
        for (unsigned d = c + 1; d + 1 < n; ++d)
          for (unsigned e = d + 1; e < n; ++e)
            best = std::max(best, referenceFive({cards[a], cards[b], cards[c], cards[d], cards[e]}, rules));
  return best;
}
