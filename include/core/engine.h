#pragma once

#include "action.h"
#include "deck.h"
#include "game_config.h"
#include "game_state.h"
#include "observation.h"
#include "pot.h"

namespace Poker {
  enum class StepStatus : uint8_t {
    NextDecision,
    HandComplete,
    InvalidAction
  };

  struct StepResult {
    StepStatus status;
    PlayerId nextPlayer;
  }

  class PokerEngine {
  public:
    PokerEngine(const GameConfig& config);
  
    void startHand();
    StepResult step(Action action);

    Observation getObservation() const;
    LegalActions getActions() const noexcept;
  
    const GameState& getState() const noexcept {
      return state_;
    }

    bool isHandComplete() const noexcept {
      return state_.status == HandStatus::Complete;
    }

  private:
    GameConfig config_;
    GameState state_;
    Deck deck_;

    uint64_t rngState_;

    void postBlinds();
    void dealHoleCards();

    void advanceStreet();
    void dealFlop();
    void dealTurn();
    void dealRiver();

    bool isBetRoundComplete() const noexcept;

    PlayerId nextActivePlayer(PlayerId from) const noexcept;
    void solveShowdown();
    void awardFreePot();
    bool validate(Action action) const noexcept;
    void applyAction(Action action);
  }
}
