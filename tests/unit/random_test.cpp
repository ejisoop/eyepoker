#include <gtest/gtest.h>

#include "pokereye/core/random.h"

#include <array>
#include <cstdint>

using namespace PokerEye;

TEST(RandomTest, SameSeedProducesSameSequence) {
  Random a(42);
  Random b(42);

  for (int i = 0; i < 10'000; i++) {
    EXPECT_EQ(a.next(), b.next());
  }
}

TEST(RandomTest, DifferentSeedsProduceDifferentSequences) {
  Random a(42);
  Random b(43);

  bool differs = false;

  for (int i = 0; i < 10'000; i++) {
    if (a.next() != b.next()) {
      differs = true;
      break;
    }
  }

  EXPECT_TRUE(differs);
}

TEST(RandomTest, BoundedValuesAreInRange) {
  Random a(42);

  for (uint64_t bound = 1; bound <= 52; bound++) {
    for (int i = 0; i < 1000; i++) {
      uint64_t value = a.bounded(bound);

      EXPECT_LT(value, bound);
    }
  }
}

TEST(RandomTest, ReseedRestoresSequence) {
  Random a(42);

  std::array<uint64_t, 100> values;

  for (auto& value : values) {
    value = a.next();
  }

  a.reseed(42);

  for (uint64_t expected : values) {
    EXPECT_EQ(a.next(), expected);
  }
}
