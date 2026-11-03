#include <pokereye/core/equity.h>
#include <pokereye/core/random.h>
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

namespace PokerEye {
namespace {
constexpr uint64_t blockSize = 4096;
constexpr uint64_t shareScale = 27720; // LCM(1..12), so every split is exact.
struct alignas(64) BlockResult {
  std::array<uint64_t, MAX_PLAYERS> wins{}, ties{}, units{}, squaredUnits{};
};
struct Prepared {
  std::array<Card, DECK_SIZE> deck{};
  unsigned size = 0;
};
Prepared prepare(const EquityRequest& r) {
  r.rules.validate();
  if (r.numPlayers < 2 || r.numPlayers > MAX_PLAYERS || r.boardCount > 5 ||
      r.trials == 0 || r.trials > 1'000'000'000 || r.threads == 0 || r.threads > 256)
    throw std::invalid_argument("invalid equity player count, board, trials (1..1e9), or threads (1..256)");
  const uint64_t allowed = ((uint64_t{1} << DECK_SIZE) - 1) &
      ~((uint64_t{1} << (r.rules.minimumRank * 4)) - 1);
  if ((r.deadCards & ~allowed) != 0) throw std::invalid_argument("dead card outside variant deck");
  uint64_t used = r.deadCards;
  auto mark = [&](Card c) {
    if (c >= DECK_SIZE || (allowed & (uint64_t{1} << c)) == 0 || (used & (uint64_t{1} << c)) != 0)
      throw std::invalid_argument("invalid, duplicate, or blocked card");
    used |= uint64_t{1} << c;
  };
  unsigned unknown = 5 - r.boardCount;
  for (unsigned p = 0; p < MAX_PLAYERS; ++p)
    for (unsigned c = 0; c < MAX_HOLE_CARDS; ++c) {
      const auto card = r.holeCards[p][c];
      if (p >= r.numPlayers || c >= r.rules.holeCards) {
        if (card != INVALID_CARD) throw std::invalid_argument("hole card outside active seats or variant");
      } else if (card == INVALID_CARD) ++unknown;
      else mark(card);
    }
  for (unsigned c = 0; c < 5; ++c) {
    if (c < r.boardCount) mark(r.board[c]);
    else if (r.board[c] != INVALID_CARD) throw std::invalid_argument("board card beyond boardCount");
  }
  Prepared result;
  for (Card c = 0; c < DECK_SIZE; ++c)
    if ((allowed & (uint64_t{1} << c)) != 0 && (used & (uint64_t{1} << c)) == 0)
      result.deck[result.size++] = c;
  if (unknown > result.size) throw std::invalid_argument("not enough remaining cards");
  return result;
}
BlockResult runBlock(const EquityRequest& r, const Prepared& prepared, uint64_t block) noexcept {
  Random rng(r.seed ^ (0x9e3779b97f4a7c15ULL * (block + 1)));
  auto deck = prepared.deck;
  auto holes = r.holeCards;
  auto board = r.board;
  BlockResult out;
  const auto trials = std::min(blockSize, r.trials - block * blockSize);
  for (uint64_t t = 0; t < trials; ++t) {
    unsigned next = 0;
    auto draw = [&]() {
      const unsigned pick = next + rng.bounded32(prepared.size - next);
      std::swap(deck[next], deck[pick]);
      return deck[next++];
    };
    for (unsigned p = 0; p < r.numPlayers; ++p)
      for (unsigned c = 0; c < r.rules.holeCards; ++c)
        if (r.holeCards[p][c] == INVALID_CARD) holes[p][c] = draw();
    for (unsigned c = r.boardCount; c < 5; ++c) board[c] = draw();
    HandRank best = 0;
    uint16_t winners = 0;
    unsigned count = 0;
    for (unsigned p = 0; p < r.numPlayers; ++p) {
      const auto rank = HandEvaluator::evaluateShowdown(holes[p], board, r.rules);
      if (count == 0 || rank > best) { best = rank; winners = 0; count = 0; }
      if (rank == best) { winners |= static_cast<uint16_t>(1u << p); ++count; }
    }
    const auto units = shareScale / count;
    for (unsigned p = 0; p < r.numPlayers; ++p)
      if ((winners & (1u << p)) != 0) {
        out.wins[p] += count == 1;
        out.ties[p] += count > 1;
        out.units[p] += units;
        out.squaredUnits[p] += units * units;
      }
  }
  return out;
}
}
EquityResult simulateEquity(const EquityRequest& request) {
  const auto prepared = prepare(request);
  const uint64_t blocks = (request.trials + blockSize - 1) / blockSize;
  const unsigned workers = static_cast<unsigned>(std::min<uint64_t>(request.threads, blocks));
  // O(workers) memory, no per-block heap allocation or shared hot counters.
  std::vector<BlockResult> partial(workers);
  auto work = [&](unsigned worker) {
    auto& total = partial[worker];
    for (uint64_t block = worker; block < blocks; block += workers) {
      const auto result = runBlock(request, prepared, block);
      for (unsigned p = 0; p < request.numPlayers; ++p) {
        total.wins[p] += result.wins[p]; total.ties[p] += result.ties[p];
        total.units[p] += result.units[p]; total.squaredUnits[p] += result.squaredUnits[p];
      }
    }
  };
  // jthread joins safely if thread creation throws after some workers started.
  {
    std::vector<std::jthread> threads;
    threads.reserve(workers - 1);
    for (unsigned w = 1; w < workers; ++w) threads.emplace_back(work, w);
    work(0);
  }
  BlockResult totals;
  for (const auto& part : partial)
    for (unsigned p = 0; p < request.numPlayers; ++p) {
      totals.wins[p] += part.wins[p]; totals.ties[p] += part.ties[p];
      totals.units[p] += part.units[p]; totals.squaredUnits[p] += part.squaredUnits[p];
    }
  EquityResult result;
  result.trials = request.trials;
  result.numPlayers = request.numPlayers;
  for (unsigned p = 0; p < request.numPlayers; ++p) {
    auto& player = result.players[p];
    player.wins = totals.wins[p]; player.ties = totals.ties[p];
    const double n = static_cast<double>(request.trials);
    player.equity = static_cast<double>(totals.units[p]) / static_cast<double>(shareScale) / n;
    const double second = static_cast<double>(totals.squaredUnits[p]) /
        static_cast<double>(shareScale * shareScale) / n;
    player.standardError = n > 1 ? std::sqrt(std::max(0.0, second - player.equity * player.equity) / (n - 1))
                                : std::numeric_limits<double>::quiet_NaN();
  }
  return result;
}
}
