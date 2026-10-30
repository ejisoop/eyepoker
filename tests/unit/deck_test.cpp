#include <gtest/gtest.h>

#include <pokereye/core/deck.h>

#include <array>
#include <cstdint>

using namespace PokerEye;

template <typename T>
class DeckTest : public ::testing::Test {};

using DeckTypes = ::testing::Types<
  Deck<ShuffleStrategy::Lazy>,
  Deck<ShuffleStrategy::Full>
>;

TYPED_TEST_SUITE(DeckTest, DeckTypes);

TYPED_TEST(DeckTest, DrawsAllCardsExactlyOnce) {
  TypeParam deck;
  Random rng(42);

  deck.reset(rng);

  std::array<bool, DECK_SIZE> seen{};

  for (int i = 0; i < DECK_SIZE; i++) {
    Card card = deck.draw(rng);

    ASSERT_LT(card, DECK_SIZE);
    EXPECT_FALSE(seen[card]);

    seen[card] = true;
  }

  EXPECT_EQ(deck.remaining(), 0);
}

TYPED_TEST(DeckTest, RemainingCountCorrect) {
  TypeParam deck;
  Random rng(42);

  deck.reset(rng);

  EXPECT_EQ(deck.remaining(), DECK_SIZE);

  for (int i = 0; i < 10; i++) {
    deck.draw(rng);
  }

  EXPECT_EQ(deck.remaining(), DECK_SIZE - 10);
}

TYPED_TEST(DeckTest, ResetRestoresAllCards) {
  TypeParam deck;
  Random rng(42);

  deck.reset(rng);

  for (int i = 0; i < 20; i++) {
    deck.draw(rng);
  }

  deck.reset(rng);

  std::array<bool, DECK_SIZE> seen{};

  for (uint8_t i = 0; i < DECK_SIZE; i++) {
    Card card = deck.draw(rng);

    ASSERT_LT(card, DECK_SIZE);
    EXPECT_FALSE(seen[card]);

    seen[card] = true;
  }

  EXPECT_EQ(deck.remaining(), 0);
}

TYPED_TEST(DeckTest, SameSeedProducesSameSequence) {
  TypeParam a;
  TypeParam b;
  Random rng_a(42);
  Random rng_b(42);
  
  a.reset(rng_a);
  b.reset(rng_b);

  for (int i = 0; i < DECK_SIZE; i++) {
    EXPECT_EQ(a.draw(rng_a), b.draw(rng_b));
  }
}

TYPED_TEST(DeckTest, RestoreAndReseedReproducesSequence) {
  TypeParam deck;
  Random rng(42);

  deck.reset(rng);

  std::array<Card, DECK_SIZE> drawn{};

  for (Card& card : drawn) {
    card = deck.draw(rng);
  }

  deck.restore();
  rng.reseed(42);
  deck.reset(rng);

  for (Card card : drawn) {
    EXPECT_EQ(deck.draw(rng), card);
  }
}
