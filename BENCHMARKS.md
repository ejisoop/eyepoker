# Benchmark and validation notes

Measured November 2, 2026 in the development sandbox, x86_64, GCC 13.3.0,
9 reported logical processors. Shared-host load and CPU details are not controlled.
Release flags: `-std=c++20 -O3 -DNDEBUG -pthread`; no `-march=native`, LTO, external
tables, or special instruction-set requirement. Timings use `steady_clock`.

## Reproduce

```sh
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release --parallel
./build/release/poker_bench
./build/release/poker_sim play --players 6 --hands 100000
```

Or without CMake:

```sh
./check.sh
g++ -std=c++20 -O3 -DNDEBUG -pthread -Iinclude src/core/*.cpp benchmarks/benchmark.cpp -o build/local/poker_bench
./build/local/poker_bench
```

The benchmark reports medians of three repetitions and prints checksums. The
seven-card corpus is generated outside the timed region and contains 16,384
hands. Equity timing includes validation, worker setup, dealing, evaluation,
and reduction. The full-game command additionally includes engine construction,
policy selection, betting, and settlement; it is a single measured run.

| Workload | Operations | Median seconds | Throughput |
| --- | ---: | ---: | ---: |
| Direct seven-card evaluation | 1,000,000 | 0.025277 | 39.56 million hands/s |
| Best of 21 five-card evaluations | 1,000,000 | 0.562906 | 1.78 million hands/s |
| Existing 64-bit modulo bounded sampler | 10,000,000 | 0.051794 | 193.07 million draws/s |
| New 32-bit multiply/reject bounded sampler | 10,000,000 | 0.023763 | 420.82 million draws/s |
| Two-player Hold'em equity, one thread | 250,000 | 0.021380 | 11.69 million trials/s |
| Two-player Hold'em equity, four threads | 250,000 | 0.006289 | 39.75 million trials/s |
| Two-player Omaha equity, one thread | 25,000 | 0.065964 | 378,994 trials/s |
| Six-player Hold'em full hands, sample policy | 100,000 | 0.192812 | 518,641 hands/s |

The direct evaluator already existed upstream; its 22.27x advantage over
21-subset enumeration is **not** a newly introduced speedup. The bounded sampler
is a new optimization and measured 2.18x faster than the upstream modulo method
for bounds 1..52. The two mapping algorithms produce different random sequences.
Four-thread equity was 3.40x the one-thread rate for this workload, with identical
win/tie counters. These short benchmarks are directional, not guarantees about
other hardware, player counts, strategy policies, or longer-run thermal behavior.
Omaha's exactly-two selection is intentionally more expensive than Hold'em.

## Validation completed locally

- GCC release compilation with `-Wall -Wextra -Wpedantic -Werror`.
- 13 self-contained test groups passed, including all 2,598,960 standard and
  376,992 Short Deck five-card hands against a separate count-and-sort oracle.
- 100,000 standard and 100,000 Short Deck seven-card samples compared against
  independent five-card subset enumeration; 5,000 additional variant samples.
- 8,000 seeded randomized full hands across Hold'em, Omaha, Omaha5 and Short Deck,
  with card uniqueness, legal actions, nonnegative stacks, and chip conservation.
- Focused heads-up, short-blind, ante, short-raise reopening, cumulative reopening,
  pot-limit, side-pot, tie, odd-chip, invalid-action, and cloning tests.
- Exact river-equity comparison, known showdown outcomes, unknown-player
  symmetry, duplicate/dead-card rejection, and worker-count determinism.
- Allocation instrumentation: **zero heap allocations** for 3,000 full engine
  constructions/copies/passive games; one fixed setup allocation for a
  single-worker equity run at both 4,096 and 40,960 trials.
- UndefinedBehaviorSanitizer quick suite passed.
- CLI equity/full-game smoke checks, duplicate-card rejection, and C++ example.

## Environment limitations

CMake, CTest, and GoogleTest are not installed in this sandbox. Source targets
were compiled directly with GCC; the CMake configuration and preserved optional
GoogleTest suite have not been executed here. The supplied GitHub workflow is
ready to validate release and sanitizer CMake builds but has not run remotely.

The full AddressSanitizer run cannot be treated as a pass: the sandbox prevents
required `/proc` inspection. ASan reports an unknown stack boundary and ignores
`__asan_handle_no_return`, followed by a stack-shadow error after exception tests;
LeakSanitizer also fails even for a standalone exception-only reproducer.
A supplemental attempt with reduced instrumentation also failed in the sanitizer
runtime. No ASan or LeakSanitizer pass is claimed. Run the unmodified sanitizer CI
job on a standard Linux runner before merging.
