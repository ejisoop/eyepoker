# PokerEye evaluation opponents

Seven original diagnostic policies for the optimized-variant-simulator branch
(tested against commit 945f20e). No external dependencies beyond your C++20 toolchain.

## Install and run on your Mac
Unzip into the repository root so `evaluation/bots.h` exists alongside `src/`.

```bash
cd ~/eyepoker
git switch bot-evaluation-league
cmake -S evaluation -B build/evaluation -DCMAKE_BUILD_TYPE=Release
cmake --build build/evaluation --parallel
./build/evaluation/poker_bot_check
./build/evaluation/poker_evaluate 100 42 64 > evaluation-results.csv
```
Arguments: independent deal seeds, master seed, Monte Carlo trials per decision.
100 seeds means 700 hands (seven rotations per seed). Larger runs:
`./build/evaluation/poker_evaluate 2000 12345 128 > evaluation-results.csv`.
The runner uses seven seats to rotate each of the seven profiles equally.

## Opponents
- random: uniformly chooses available action types; raises roughly half-pot.
- calling_station: checks or calls every time.
- tight: demands a larger equity advantage before calling or raising.
- loose: calls with a deliberately lower equity threshold.
- aggressive: lower value-raise threshold, larger sizing, occasional bluffs.
- short_shove: switches to shove-or-fold below 12 big blinds.
- equity: neutral pot-odds / equity baseline.

`Bot::act` takes only Observation, public rules, blind size, and sample count.
It cannot access the engine, opponent hole cards, deck order or engine RNG.
Raise amounts are raise-to totals and clipped to the supplied legal bounds.
All active opponents, including all-ins, are included in equity sampling.
Only the runner reads final engine state to score the results.

## Interpretation and limits
This is an OFFLINE C++ evaluation pack, not the competition's Python Agent.
Each hand starts with fresh 100-BB stacks. This is not a sit-and-go runner,
ICM policy, blind schedule, tournament score predictor, or opponent-range model.
Equity opponents sample uniform unknown hands. They ignore range conditioning,
side-pot-specific equity, position and future betting. They are intentionally
simple diagnostic baselines, not established strong poker players.
The public rules object supports other community-card variants; check.cpp tests
Hold'em, Omaha and short deck where table size fits the deck.

CSV reports big blinds per 100 hands, standard error across independent seed
clusters (each cluster averages seven seat rotations), decision counts, mean and
maximum observed microseconds. Timing includes equity sampling. C++ timings do
not certify compliance with a Python sandbox's 1-ms deadline. There is no timeout
enforcement here; illegal actions fail the run immediately.
Shared seeds/rotations reduce seating bias, but adaptive draw consumption can
cause different future boards after different actions. Small-run rankings are
not reliable strength estimates. Use held-out seeds and broader opponents.

To add a candidate, extend the profiles and runner dimensions together, or replace
one profile while retaining its index and seed schedule for matched comparisons.
For a full tournament, add persistent stacks, blinds, elimination order and payout
scoring after verifying the official simulator/interface.

## Checks
`poker_bot_check` runs complete hands across 2/7/8/12 players, 3/10/100-BB stacks,
and three variants where legal; fails on illegal actions or chip loss.
A Linux GCC C++20 build and the seat-rotation smoke run were checked during creation.
