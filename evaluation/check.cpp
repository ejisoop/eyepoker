#include "bots.h"
#include <pokereye/core/engine.h>
#include <iostream>
#include <vector>
using namespace PokerEye;
int main() {
 try {
  unsigned hands=0;
  for(auto rules:{GameRules::holdem(),GameRules::omaha(),GameRules::shortDeck()})
   for(unsigned n:{2u,7u,8u,12u}) for(unsigned depth:{3u,10u,100u}) {
    if(n*rules.holeCards+5 > (13-rules.minimumRank)*4) continue;
    GameConfig c;c.rules=rules;c.numPlayers=n;c.bigBlind=20;c.smallBlind=10;
    c.startingStack=depth*20;c.seed=100+hands;
    PokerEngine e(c);e.startHand(); unsigned actions=0;
    std::vector<Evaluation::Bot> bots;
    for(unsigned s=0;s<n;++s) bots.emplace_back(s%7,c.seed+s);
    while(!e.isHandComplete()) {
      if(++actions>10000) throw std::logic_error("too many actions");
      auto o=e.getObservation();auto a=bots[o.hero].act(o,c.rules,20,16);
      if(e.step(a).status==StepStatus::InvalidAction) throw std::logic_error("illegal action");
    }
    Chips sum=0;for(unsigned s=0;s<n;++s) sum+=e.getState().players[s].stack;
    if(sum!=n*c.startingStack) throw std::logic_error("lost chips");
    ++hands;
   }
  std::cout<<hands<<" integration hands passed across variants, player counts, and stack depths\n";
 } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
