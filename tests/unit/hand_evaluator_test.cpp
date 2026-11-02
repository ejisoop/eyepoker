#include <gtest/gtest.h>
#include <pokereye/core/hand_evaluator.h>
#include <pokereye/core/card.h>
#include <algorithm>
#include <numeric>
#include <random>

using namespace PokerEye;

static HandRank reference(const std::array<Card, 5>& cards) {
  unsigned counts[13]{};
  bool flush = true;
  for (Card c : cards) {
    ++counts[rank(c)];
    flush &= suit(c) == suit(cards[0]);
  }
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
  if (straight < 0 && counts[12] && counts[0] && counts[1] && counts[2] && counts[3]) straight = 3;
  unsigned category = 0;
  std::array<unsigned, 5> tie{};
  if (straight >= 0 && flush) { category = 8; tie[0] = straight; }
  else if (counts[ranks[0]] == 4) { category = 7; tie[0] = ranks[0]; tie[1] = ranks[1]; }
  else if (counts[ranks[0]] == 3 && counts[ranks[1]] == 2) {
    category = 6; tie[0] = ranks[0]; tie[1] = ranks[1];
  } else if (flush) { category = 5; std::copy_n(ranks.begin(), 5, tie.begin()); }
  else if (straight >= 0) { category = 4; tie[0] = straight; }
  else {
    category = counts[ranks[0]] == 3 ? 3 : counts[ranks[0]] == 2 ? (counts[ranks[1]] == 2 ? 2 : 1) : 0;
    const unsigned n = category == 3 ? 3 : category == 2 ? 3 : category == 1 ? 4 : 5;
    std::copy_n(ranks.begin(), n, tie.begin());
  }
  HandRank result = category << 20;
  for (unsigned i = 0; i < 5; ++i) result |= tie[i] << (16 - 4 * i);
  return result;
}

TEST(HandEvaluator, AllFiveCardHandsMatchIndependentReference) {
  std::array<unsigned, 9> counts{};
  for (Card a = 0; a < 48; ++a)
    for (Card b = a + 1; b < 49; ++b)
      for (Card c = b + 1; c < 50; ++c)
        for (Card d = c + 1; d < 51; ++d)
          for (Card e = d + 1; e < 52; ++e) {
            const std::array<Card, 5> cards{a,b,c,d,e};
            const auto actual = HandEvaluator::evaluateFive(cards);
            ASSERT_EQ(actual, reference(cards));
            ++counts[actual >> 20];
          }
  EXPECT_EQ(counts, (std::array<unsigned, 9>{1302540,1098240,123552,54912,10200,5108,3744,624,40}));
}

TEST(HandEvaluator, SevenMatchesBestOfTwentyOneIndependentHands) {
  std::mt19937 rng(42);
  std::array<Card, 52> deck;
  std::iota(deck.begin(), deck.end(), 0);
  for (unsigned trial = 0; trial < 100000; ++trial) {
    for (unsigned i = 0; i < 7; ++i) {
      std::uniform_int_distribution<unsigned> pick(i, 51);
      std::swap(deck[i], deck[pick(rng)]);
    }
    std::array<Card, 7> cards;
    std::copy_n(deck.begin(), 7, cards.begin());
    HandRank expected = 0;
    for (unsigned a = 0; a < 6; ++a)
      for (unsigned b = a + 1; b < 7; ++b) {
        std::array<Card, 5> five;
        unsigned n = 0;
        for (unsigned i = 0; i < 7; ++i) if (i != a && i != b) five[n++] = cards[i];
        expected = std::max(expected, reference(five));
      }
    ASSERT_EQ(HandEvaluator::evaluateSeven(cards), expected) << "trial " << trial;
    std::reverse(cards.begin(), cards.end());
    ASSERT_EQ(HandEvaluator::evaluateSeven(cards), expected);
  }
}

TEST(Card, EncodingRoundTrips) {
  for (unsigned r = 0; r < 13; ++r)
    for (unsigned s = 0; s < 4; ++s) {
      EXPECT_EQ(rank(newCard(r, s)), r);
      EXPECT_EQ(suit(newCard(r, s)), s);
    }
}
