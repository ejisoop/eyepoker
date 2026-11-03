#pragma once
#include <pokereye/core/observation.h>
#include <pokereye/core/equity.h>
#include <pokereye/core/random.h>
#include <algorithm>
#include <bit>
#include <string_view>
#include <stdexcept>

namespace Evaluation {
using namespace PokerEye;
inline constexpr std::string_view names[] = {
  "random", "calling_station", "tight", "loose", "aggressive", "short_shove", "equity"
};
// Offline test opponents. Equity is versus uniform unknown hands, not inferred ranges.
class Bot {
  unsigned kind_;
  Random rng_;
public:
  Bot(unsigned kind, uint64_t seed) : kind_(kind), rng_(seed) {
    if (kind >= 7) throw std::invalid_argument("bot index must be 0..6");
  }
  Action act(const Observation& o, const GameRules& rules, Chips bigBlind,
             uint64_t trials = 64) {
    const auto& l = o.legal;
    auto passive = [&]() -> Action {
      if(l.allows(CanCheck)) return {ActionType::Check};
      if(l.allows(CanCall)) return {ActionType::Call};
      if(l.allows(CanFold)) return {ActionType::Fold};
      throw std::logic_error("no passive legal action");
    };
    auto fold = [&]() -> Action {
      if(l.allows(CanCheck)) return {ActionType::Check};
      if(l.allows(CanFold)) return {ActionType::Fold};
      return passive();
    };
    auto raise = [&](double fraction, bool shove=false) -> Action {
      if(!l.allows(CanRaise)) return passive();
      Chips target = shove ? l.maxRaiseTo : o.streetBets[o.hero] + l.toCall +
          static_cast<Chips>(fraction * (o.pot + l.toCall));
      return {ActionType::Raise, std::clamp(target, l.minRaiseTo, l.maxRaiseTo)};
    };
    if(kind_ == 0) {
      Action choices[4]; unsigned n=0;
      if(l.allows(CanFold)) choices[n++]={ActionType::Fold};
      if(l.allows(CanCheck)) choices[n++]={ActionType::Check};
      if(l.allows(CanCall)) choices[n++]={ActionType::Call};
      if(l.allows(CanRaise)) choices[n++]=raise(0.5);
      if(!n) throw std::logic_error("no legal action");
      return choices[rng_.bounded32(n)];
    }
    if(kind_ == 1) return passive();
    EquityRequest r;
    r.rules=rules; r.numPlayers=static_cast<uint8_t>(std::popcount(o.activeMask));
    if(r.numPlayers < 2) return passive();
    r.holeCards[0]=o.holeCards; r.board=o.board; r.boardCount=o.boardCount;
    r.trials=trials; r.seed=rng_.next(); r.threads=1;
    const double e=simulateEquity(r).players[0].equity;
    const double fair=1.0/r.numPlayers;
    const Chips cost=std::min(l.toCall,o.stacks[o.hero]);
    const double odds=cost ? double(cost)/double(o.pot+cost) : 0;
    // Tight and loose profiles intentionally bracket the neutral equity baseline.
    const double margin=kind_==2 ? .13 : kind_==3 ? -.08 : .025;
    const double value=kind_==2 ? 1.65 : kind_==3 ? 1.2 : kind_==4 ? 1.1 : 1.35;
    if(kind_==5 && o.stacks[o.hero] <= 12*bigBlind) {
      if(e > std::max(odds+.08,fair*1.25)) return raise(1,true);
      return fold();
    }
    if(e > std::max(fair*value,odds+.15)) return raise(kind_==4 ? 1 : .65);
    if(kind_==4 && rng_.bounded32(100)<20) return raise(.75);
    if(l.allows(CanCheck)) return passive();
    return e >= odds+margin ? passive() : fold();
  }
};
}
