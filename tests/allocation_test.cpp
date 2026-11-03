#include <pokereye/core/engine.h>
#include <pokereye/core/equity.h>
#include <cstdlib>
#include <iostream>
#include <new>

namespace { std::size_t allocations = 0; }
void* operator new(std::size_t bytes) {
  ++allocations;
  if (void* memory = std::malloc(bytes ? bytes : 1)) return memory;
  throw std::bad_alloc();
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }
// Aligned allocations matter for the cache-line-aligned equity accumulators.
void* operator new(std::size_t bytes, std::align_val_t alignment) {
  ++allocations;
  const auto a = static_cast<std::size_t>(alignment);
  const auto size = ((bytes ? bytes : 1) + a - 1) / a * a;
  if (void* memory = std::aligned_alloc(a, size)) return memory;
  throw std::bad_alloc();
}
void operator delete(void* memory, std::align_val_t) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t, std::align_val_t) noexcept { std::free(memory); }
int main() {
  using namespace PokerEye;
  const auto before = allocations;
  for (auto rules : {GameRules::holdem(), GameRules::omaha(), GameRules::shortDeck()}) {
    GameConfig config; config.rules = rules; config.numPlayers = 4;
    for (unsigned i = 0; i < 1000; ++i) {
      config.seed = i;
      PokerEngine engine(config); engine.startHand();
      auto copy = engine;
      while (!copy.isHandComplete()) {
        const auto obs = copy.getObservation();
        if (copy.step({obs.legal.allows(CanCheck) ? ActionType::Check : ActionType::Call}).status == StepStatus::InvalidAction) return 1;
      }
    }
  }
  const auto used = allocations - before;
  std::cout << "Full-game hot-path allocations: " << used << '\n';
  if (used != 0) return 1;
  EquityRequest r; r.trials = 4096;
  auto start = allocations;
  (void)simulateEquity(r);
  const auto small = allocations - start;
  r.trials = 40960;
  start = allocations;
  (void)simulateEquity(r);
  const auto large = allocations - start;
  std::cout << "Single-thread equity allocations: " << small << " (4096 trials), " << large << " (40960 trials)\n";
  return small == large ? 0 : 1;
}
