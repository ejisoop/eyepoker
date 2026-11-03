#include <pokereye/core/engine.h>
#include <pokereye/core/equity.h>
#include <iostream>

int main() {
  using namespace PokerEye;
  GameConfig config;
  config.rules = GameRules::omaha(5);
  config.rules.betting = BettingStructure::NoLimit; // One-line rules adaptation.
  config.numPlayers = 4;
  config.seed = 123;
  PokerEngine engine(config);
  engine.startHand();
  while (!engine.isHandComplete()) {
    const auto observation = engine.getObservation();
    const Action action{observation.legal.allows(CanCheck) ? ActionType::Check : ActionType::Call};
    if (engine.step(action).status == StepStatus::InvalidAction) return 1;
  }
  for (unsigned i = 0; i < config.numPlayers; ++i)
    std::cout << "seat " << i << ": " << engine.getState().players[i].stack << '\n';

  EquityRequest request;
  request.rules = GameRules::shortDeck();
  request.holeCards[0][0] = newCard(12, 0);
  request.holeCards[0][1] = newCard(12, 1);
  request.threads = 4;
  const auto equity = simulateEquity(request);
  std::cout << "Short Deck AA equity: " << equity.players[0].equity << '\n';
}
