#include <pokereye/core/deck.h>
#include <pokereye/core/hand_evaluator.h>
#include <pokereye/core/equity.h>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace PokerEye;
namespace {
constexpr unsigned iterations = 1'000'000;
HandRank subsets(const std::array<Card, 7>& cards) {
  HandRank best = 0;
  for (unsigned a = 0; a < 6; ++a)
    for (unsigned b = a + 1; b < 7; ++b) {
      std::array<Card, 5> five{}; unsigned n = 0;
      for (unsigned i = 0; i < 7; ++i) if (i != a && i != b) five[n++] = cards[i];
      best = std::max(best, HandEvaluator::evaluateFive(five));
    }
  return best;
}
template<class F> double measure(const char* name, uint64_t n, F fn) {
  // Three repeats, report median. Printed checksum prevents dead-code removal.
  std::array<double, 3> times{}; uint64_t checksum = 0;
  for (auto& seconds : times) {
    const auto start = std::chrono::steady_clock::now();
    checksum ^= fn();
    seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  }
  std::sort(times.begin(), times.end());
  std::cout << name << ',' << n << ',' << times[1] << ',' << static_cast<double>(n) / times[1] << ',' << checksum << '\n';
  return times[1];
}
}
int main() {
  std::cout << std::fixed << std::setprecision(6);
  std::cout << "benchmark,operations,median_seconds,operations_per_second,checksum\n";
  std::vector<std::array<Card, 7>> corpus(16384);
  Random rng(42); Deck<> deck;
  for (auto& cards : corpus) { deck.reset(rng); for (auto& c : cards) c = deck.draw(rng); }
  for (const auto& cards : corpus) if (subsets(cards) != HandEvaluator::evaluateSeven(cards)) return 1;
  const auto fast = measure("direct_seven", iterations, [&] {
    uint64_t sum = 0;
    for (unsigned i = 0; i < iterations; ++i) sum += HandEvaluator::evaluateSeven(corpus[i % corpus.size()]);
    return sum;
  });
  const auto baseline = measure("enumerate_21_fives", iterations, [&] {
    uint64_t sum = 0;
    for (unsigned i = 0; i < iterations; ++i) sum += subsets(corpus[i % corpus.size()]);
    return sum;
  });
  std::cout << "direct_vs_enumeration_speedup=" << baseline / fast << '\n';
  const auto oldRng = measure("bounded64_modulo", iterations * 10, [] {
    Random r(42); uint64_t sum = 0;
    for (unsigned i = 0; i < iterations * 10; ++i) sum += r.bounded(1 + (i % 52));
    return sum;
  });
  const auto newRng = measure("bounded32_multiply_reject", iterations * 10, [] {
    Random r(42); uint64_t sum = 0;
    for (unsigned i = 0; i < iterations * 10; ++i) sum += r.bounded32(1 + (i % 52));
    return sum;
  });
  std::cout << "bounded_sampler_speedup=" << oldRng / newRng << '\n';
  for (const auto threads : {1u, 4u}) {
    EquityRequest r; r.trials = 250'000; r.threads = threads;
    measure(threads == 1 ? "holdem_equity_1_thread" : "holdem_equity_4_threads", r.trials, [&] {
      const auto result = simulateEquity(r);
      return result.players[0].wins + result.players[0].ties;
    });
  }
  EquityRequest omaha; omaha.rules = GameRules::omaha(); omaha.trials = 25'000;
  measure("omaha_equity_1_thread", omaha.trials, [&] { return simulateEquity(omaha).players[0].wins; });
}
