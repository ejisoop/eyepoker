# PokerEye

A dependency-free C++20 simulator for community-card high poker: full betting
hands and parallel Monte Carlo showdown equity. Configurable rules keep deck,
hand-selection, ranking, and betting choices separate from the state machine.

## Build and run

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --parallel
ctest --test-dir build/release --output-on-failure

# AA vs KK, reproducible across thread counts
./build/release/poker_sim equity --hand AsAh --hand KsKh --trials 1000000 --threads 4

# Omaha on a known flop; second player's hand is uniformly random
./build/release/poker_sim equity --variant omaha --hand AsAhKdQc --board 2c3d4h --trials 100000

# Short Deck, three opponents, with blocked cards
./build/release/poker_sim equity --variant shortdeck --players 4 --hand AsAh --dead KsKd

# Full six-player betting simulations, random legal decisions
./build/release/poker_sim play --players 6 --hands 100000
```

Card notation is `23456789TJQKA` plus `cdhs`. Repeated `--hand` options assign
seats in order; `??` marks an unknown hole card, and omitted seats are random.
Duplicate, blocked, out-of-deck, or excess cards are rejected. Use `--help` for
all options. No CMake installed? `./check.sh` builds and runs the tests, CLI,
and example using just Bash and a C++20 compiler (`CXX` defaults to `g++`).

## Supported rules

| Preset | Hole cards | Selection | Deck | Betting |
| --- | ---: | --- | --- | --- |
| `GameRules::holdem()` | 2 | Best five of all seven | 52 | No-limit |
| `GameRules::omaha()` | 4 | Exactly two hole + three board | 52 | Pot-limit |
| `GameRules::omaha(5)` / `(6)` | 5 / 6 | Exactly two hole + three board | 52 | Pot-limit |
| `GameRules::shortDeck()` | 2 | Best five of all seven | 36 (6–A) | No-limit |

Short Deck uses A-6-7-8-9 as its low straight, flush over full house, and straight
over trips. These are explicit house-rule choices; `tripsBeatStraight` and
`flushBeatsFullHouse` are configurable. Every preset uses ordinary small/big
blinds and optional individual antes. A button-ante Short Deck format is **not**
implemented. `burnCards` defaults to true for full hands; equity integrates over
unseen burns, so it need not draw them.

To adapt a variant, copy a preset and change fields:

```cpp
#include <pokereye/core/engine.h>

PokerEye::GameConfig config;
config.rules = PokerEye::GameRules::omaha(5);
config.rules.betting = PokerEye::BettingStructure::NoLimit;
config.numPlayers = 4;
config.ante = 25;
config.seed = 123;
PokerEye::PokerEngine engine(config);
engine.startHand();
while (!engine.isHandComplete()) {
    const auto view = engine.getObservation();
    const auto type = view.legal.allows(PokerEye::CanCheck)
        ? PokerEye::ActionType::Check : PokerEye::ActionType::Call;
    engine.step({type});
}
```

See [`examples/custom_variant.cpp`](examples/custom_variant.cpp) for both engine
and equity APIs. Link `PokerEye::core` from CMake. The core has no external
runtime dependencies beyond the C++ standard library and system threads.

## Engine contract

- `startHand()` keeps current stacks and rotates the button among funded seats.
  Call `canStartHand()` first when simulating tournaments. A live hand cannot be
  replaced; insufficient funded seats or a live hand cause `std::logic_error`.
- All-zero `initialStacks` selects equal `startingStack` values; otherwise the
  array defines exact seat stacks, with zero-stack seats sitting out.
- `Action{ActionType::Raise, amount}` means **raise to that street total**.
  An opening bet also uses `Raise`. `minRaiseTo` includes a legal short all-in
  when the player cannot afford a full raise. `Call` automatically caps at stack.
- Invalid actions return `InvalidAction` without changing state or RNG. Check
  is permitted only with nothing owed; fold is permitted only when facing a bet.
  Non-raise actions must leave `Action::raise` at zero.
- Handles heads-up order, big-blind options, short all-ins, cumulative reopening,
  pot-limit caps (including nominal short blinds preflop), all-in runouts,
  folded-player contributions, side pots, uncalled excess, and odd chips left
  of the button. Uses a **moving button** among funded seats; tournament dead-
  button policies and table reseating are outside this engine.
- `getObservation()` exposes the acting player's cards and public information.
  `getState()` is an omniscient inspection API; do not pass it to a policy that
  is intended to have imperfect information.
- At completion `pot == 0`, stacks include awards, and `winnings` records gross
  pot awards including returned uncalled chips. `totalBet` retains contributions
  for that hand. Net gain is final stack minus starting stack.
- Engine copies capture the entire state, deck and RNG, allowing deterministic
  branching. Instances have no shared mutable state; use one per worker.

## Equity and reproducibility

`simulateEquity(EquityRequest)` samples uniformly from legal unknown cards,
including partially known hands and dead cards. It reports outright wins, ties,
fractional-pot equity, and sample standard error. For a single trial the
standard error is NaN because it cannot be estimated. These are equally contested
showdown equities; they do **not** model folds, unequal stacks, weighted ranges,
strategy, rake, or side-pot equity. Use the betting engine for actual pot settlement.

Each deterministic 4,096-trial block has a separate seed derived from the request.
Integer accumulators represent splits exactly using LCM(1..12), and reduction is
order-independent. Worker counts and scheduling therefore do not change results.
Storage is O(workers); no heap allocations occur inside trial loops. Supported
bounds are 1–1,000,000,000 trials and 1–256 requested threads; fewer threads are
used when there are fewer blocks. Changing the seed changes the sample.

The original xoshiro256++ generator is retained. Deck sampling now uses unbiased
32-bit multiply/reject mapping; deck sequences differ from the original modulo
sampler even with the same seed. Replays are deterministic within this version.
The PRNG is for simulation, **not** cryptographic dealing.

## Performance

- Retains the upstream direct bitmask seven-card evaluator: no 21-subset search
  for ordinary Hold'em or Short Deck.
- Uses fixed-size state, cards, rank masks and pot accumulators; full-hand engine
  construction, copying, observations and steps allocate no heap memory.
- Lazy partial Fisher–Yates draws only required cards; full shuffle remains
  selectable. Multiply/reject sampling removes common-path integer division.
- Parallel equity uses private RNG/deck state and cache-line-aligned accumulators,
  with no shared counters or locks in the simulation loop.
- Omaha enumerates the required 2+3 selections. Custom `AnyFive` games with more
  than two hole cards use a general five-card-combination fallback. These paths
  prioritize transparent correctness and are slower than the direct evaluator.

Run `poker_bench` in a release build. `-DPOKEREYE_NATIVE=ON` optionally tunes for
the build machine; the default stays portable. See [BENCHMARKS.md](BENCHMARKS.md)
for measured throughput, comparison scope, and verification limitations. No claim
is made that this is the fastest implementation across all hardware or variants.

## Tests and extension boundaries

The default test suite runs offline and covers all 2,598,960 standard five-card
hands, all 376,992 Short Deck five-card hands, 200,000 seven-card hands against an
independent five-card oracle, 5,000 variant hands, deterministic multithreading,
exact-river equity comparison, and 8,000 randomized full games with chip and card
invariants. Separate allocation tests enforce the no-heap engine contract.

`-DPOKEREYE_SANITIZERS=ON` enables ASan/UBSan. The GitHub workflow checks release
and sanitizer builds. Original GoogleTest tests are preserved and can also run
with `-DPOKEREYE_LEGACY_TESTS=ON` when GTest is installed; configuration never
fetches test dependencies from the network.

Current scope is single-board, high-only community-card games with 2–6 hole
cards and 2–12 seats, subject to deck capacity. Stud/draw games, fixed-limit,
hi/lo splits, multi-board games, straddles, rake, weighted ranges, exact exhaustive
preflop equity, and strategy solving require additional modules. Evaluator and
`PotManager` hot-path APIs assume valid distinct cards/rules and valid engine
states; the engine constructor and equity API validate external inputs.

Betting behavior references the [Poker TDA rules](https://www.pokertda.com/view-poker-tda-rules/)
for minimum raises, reopening, pot-limit blind calculations, and separate side
pots. The bounded sampler uses [Lemire's multiply/reject method](https://lemire.me/en/publication/arxiv1805/).
