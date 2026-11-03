#include "bots.h"
#include <pokereye/core/engine.h>
#include <chrono>
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
using namespace PokerEye;
using namespace Evaluation;
int main(int argc,char** argv) {
 try {
  const unsigned deals=argc>1 ? std::stoul(argv[1]) : 100;
  const uint64_t seed=argc>2 ? std::stoull(argv[2]) : 42;
  const uint64_t trials=argc>3 ? std::stoull(argv[3]) : 64;
  if(!deals || deals>1000000 || !trials || trials>100000) throw std::invalid_argument("invalid run size");
  GameConfig c; c.numPlayers=7; c.smallBlind=10; c.bigBlind=20; c.startingStack=2000;
  double sums[7]{},squares[7]{},micros[7]{},maxMicros[7]{};
  uint64_t decisions[7]{}; Random seeds(seed);
  for(unsigned d=0;d<deals;++d) {
    const auto dealSeed=seeds.next(); double block[7]{};
    // Each seed is a cluster: seven rotations, each bot occupying every seat.
    for(unsigned rotation=0;rotation<7;++rotation) {
      c.seed=dealSeed; PokerEngine engine(c); engine.startHand();
      std::vector<Bot> bots;
      for(unsigned b=0;b<7;++b) bots.emplace_back(b,dealSeed ^ (uint64_t(b+1)*0x9e3779b97f4a7c15ULL));
      unsigned steps=0;
      while(!engine.isHandComplete()) {
        if(++steps>10000) throw std::logic_error("hand exceeded decision guard");
        const auto o=engine.getObservation(); const unsigned b=(o.hero+rotation)%7;
        const auto begin=std::chrono::steady_clock::now();
        const auto action=bots[b].act(o,c.rules,c.bigBlind,trials);
        const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count();
        micros[b]+=us; maxMicros[b]=std::max(maxMicros[b],us); ++decisions[b];
        if(engine.step(action).status==StepStatus::InvalidAction) throw std::logic_error("illegal bot action");
      }
      Chips total=0;
      for(unsigned s=0;s<7;++s) {
        const auto stack=engine.getState().players[s].stack; total+=stack;
        block[(s+rotation)%7]+=double(stack-c.startingStack)/c.bigBlind/7;
      }
      if(total!=7*c.startingStack) throw std::logic_error("chip conservation failure");
    }
    for(unsigned b=0;b<7;++b) { sums[b]+=block[b]; squares[b]+=block[b]*block[b]; }
  }
  std::cout<<"bot,hands,bb_per_100,cluster_standard_error_bb100,decisions,mean_us,max_us\n"<<std::fixed<<std::setprecision(3);
  for(unsigned b=0;b<7;++b) {
    const double mean=sums[b]/deals;
    const double se=deals>1 ? std::sqrt(std::max(0.0,squares[b]-deals*mean*mean)/(deals-1)/deals) : NAN;
    std::cout<<names[b]<<','<<deals*7<<','<<100*mean<<','<<100*se<<','<<decisions[b]<<','
      <<(decisions[b]?micros[b]/decisions[b]:0)<<','<<maxMicros[b]<<'\n';
  }
 } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
