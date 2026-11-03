#pragma once

#include "action.h"
#include "deck.h"
#include "game_config.h"
#include "game_state.h"
#include "observation.h"
#include "pot.h"

namespace PokerEye {
enum class StepStatus : uint8_t { NextDecision, HandComplete, InvalidAction };
struct StepResult {
  StepStatus status;
  PlayerId nextPlayer;
};

// Copyable, allocation-free state machine. Each instance owns its RNG and deck.
// Use one instance per worker; sharing a mutable engine is not thread-safe.
class PokerEngine {
public:
  explicit PokerEngine(const GameConfig& config = {});
  void startHand(); // Keeps stacks, rotates the button; throws if a hand is live.
  bool canStartHand() const noexcept;
  StepResult step(Action action) noexcept;
  Observation getObservation() const noexcept;
  LegalActions getActions() const noexcept;
  const GameState& getState() const noexcept { return state_; }
  bool isHandComplete() const noexcept { return state_.status == HandStatus::Complete; }

private:
  GameConfig config_;
  GameState state_{};
  Random rng_;
  Deck<ShuffleStrategy::Lazy> lazyDeck_;
  Deck<ShuffleStrategy::Full> fullDeck_;
  Chips preflopPotCorrection_ = 0;

  Card draw() noexcept;
  PlayerId nextDealt(PlayerId from) const noexcept;
  PlayerId nextDecision(PlayerId from) const noexcept;
  void pay(PlayerId player, Chips amount, bool street = true) noexcept;
  void advanceStreet() noexcept;
  void advance(PlayerId from) noexcept;
  void finish(bool showdown) noexcept;
  bool validate(Action action) const noexcept;
};
}
