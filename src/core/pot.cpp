#include <pokereye/core/pot.h>
#include <algorithm>
#include <cassert>
#include <limits>

namespace PokerEye {
PotResult PotManager::settle(const GameState& state,
                            const std::array<HandRank, MAX_PLAYERS>& ranks) noexcept {
  PotResult result;
  Chips previous = 0;
  for (;;) {
    Chips level = std::numeric_limits<Chips>::max();
    for (unsigned i = 0; i < state.numPlayers; ++i)
      if (state.players[i].totalBet > previous) level = std::min(level, state.players[i].totalBet);
    if (level == std::numeric_limits<Chips>::max()) break;
    unsigned contributors = 0, winners = 0;
    PlayerId sole = INVALID_PLAYER;
    uint16_t winnerMask = 0;
    HandRank best = 0;
    for (PlayerId i = 0; i < state.numPlayers; ++i) {
      const auto& p = state.players[i];
      if (p.totalBet < level) continue;
      ++contributors;
      sole = i;
      if (p.status != PlayerStatus::Active && p.status != PlayerStatus::AllIn) continue;
      if (winners == 0 || ranks[i] > best) { best = ranks[i]; winnerMask = 0; winners = 0; }
      if (ranks[i] == best) { winnerMask |= static_cast<uint16_t>(1u << i); ++winners; }
    }
    const Chips amount = (level - previous) * contributors;
    if (contributors == 1) result.winnings[sole] += amount; // Uncalled excess.
    else {
      assert(winners > 0); // Valid engine states always have an eligible player.
      if (winners == 0) return result;
      const Chips share = amount / winners;
      Chips extra = amount % winners;
      // Each side pot splits independently; odd chips start left of the button.
      for (unsigned n = 1; n <= state.numPlayers; ++n) {
        const auto i = (state.dealerButton + n) % state.numPlayers;
        if ((winnerMask & (1u << i)) != 0) {
          result.winnings[i] += share + (extra > 0 ? 1 : 0);
          if (extra > 0) --extra;
        }
      }
    }
    previous = level;
  }
  return result;
}
}
