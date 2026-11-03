#include <pokereye/core/engine.h>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace PokerEye {
namespace {
bool live(PlayerStatus s) noexcept {
  return s == PlayerStatus::Active || s == PlayerStatus::AllIn;
}
}

PokerEngine::PokerEngine(const GameConfig& config)
    : config_(config), rng_(config.seed), lazyDeck_(config.rules.minimumRank),
      fullDeck_(config.rules.minimumRank) {
  config_.rules.validate();
  // Reserve arithmetic headroom for pot-limit calculations, minimum raises,
  // and accumulated payouts. All chips remain within this initial table total.
  constexpr Chips limit = std::numeric_limits<Chips>::max() / (8 * MAX_PLAYERS);
  if (config.numPlayers < 2 || config.numPlayers > MAX_PLAYERS ||
      config.smallBlind < 0 || config.bigBlind <= 0 || config.smallBlind > config.bigBlind ||
      config.bigBlind > limit || config.ante < 0 || config.ante > limit ||
      config.startingStack < 0 || config.startingStack > limit ||
      (config.shuffleStrategy != ShuffleStrategy::Lazy && config.shuffleStrategy != ShuffleStrategy::Full))
    throw std::invalid_argument("invalid player count, stakes, stack, or shuffle strategy");
  if (config.numPlayers * config.rules.holeCards + 5 + (config.rules.burnCards ? 3 : 0) >
      (13 - config.rules.minimumRank) * 4)
    throw std::invalid_argument("not enough cards for this table and variant");
  bool custom = false;
  for (unsigned i = 0; i < MAX_PLAYERS; ++i) {
    const Chips stack = config.initialStacks[i];
    if (stack < 0 || stack > limit || (i >= config.numPlayers && stack != 0))
      throw std::invalid_argument("invalid initial seat stack");
    custom |= stack != 0;
  }
  state_.numPlayers = config.numPlayers;
  state_.holeCardCount = config.rules.holeCards;
  for (unsigned i = 0; i < config.numPlayers; ++i)
    state_.players[i].stack = custom ? config.initialStacks[i] : config.startingStack;
  if (!canStartHand()) throw std::invalid_argument("at least two funded seats are required");
}

bool PokerEngine::canStartHand() const noexcept {
  if (state_.status == HandStatus::InProgress) return false;
  unsigned funded = 0;
  for (unsigned i = 0; i < state_.numPlayers; ++i) funded += state_.players[i].stack > 0;
  return funded >= 2;
}

Card PokerEngine::draw() noexcept {
  return config_.shuffleStrategy == ShuffleStrategy::Lazy ? lazyDeck_.draw(rng_) : fullDeck_.draw(rng_);
}

PlayerId PokerEngine::nextDealt(PlayerId from) const noexcept {
  for (unsigned n = 1; n <= state_.numPlayers; ++n) {
    const auto id = static_cast<PlayerId>((from + n) % state_.numPlayers);
    if (live(state_.players[id].status)) return id;
  }
  return INVALID_PLAYER;
}

PlayerId PokerEngine::nextDecision(PlayerId from) const noexcept {
  for (unsigned n = 1; n <= state_.numPlayers; ++n) {
    const auto id = static_cast<PlayerId>((from + n) % state_.numPlayers);
    const auto& p = state_.players[id];
    if (p.status == PlayerStatus::Active && p.needsAction) return id;
  }
  return INVALID_PLAYER;
}

void PokerEngine::pay(PlayerId id, Chips amount, bool street) noexcept {
  auto& p = state_.players[id];
  amount = std::min(amount, p.stack);
  p.stack -= amount;
  p.totalBet += amount;
  if (street) p.streetBet += amount;
  state_.pot += amount;
  if (p.stack == 0) { p.status = PlayerStatus::AllIn; p.needsAction = false; }
}

void PokerEngine::startHand() {
  if (!canStartHand()) throw std::logic_error("cannot start: live hand or fewer than two funded seats");
  const auto priorButton = state_.dealerButton;
  const auto number = state_.handNumber + 1;
  std::array<Chips, MAX_PLAYERS> stacks{};
  for (unsigned i = 0; i < state_.numPlayers; ++i) stacks[i] = state_.players[i].stack;
  state_ = GameState{};
  state_.numPlayers = config_.numPlayers;
  state_.holeCardCount = config_.rules.holeCards;
  state_.handNumber = number;
  state_.status = HandStatus::InProgress;
  unsigned dealt = 0;
  for (unsigned i = 0; i < state_.numPlayers; ++i) {
    auto& p = state_.players[i];
    p.stack = stacks[i];
    p.status = p.stack ? PlayerStatus::Active : PlayerStatus::SittingOut;
    p.needsAction = p.stack > 0;
    dealt += p.stack > 0;
  }
  state_.dealerButton = nextDealt(priorButton == INVALID_PLAYER
      ? static_cast<PlayerId>(state_.numPlayers - 1) : priorButton);
  if (config_.shuffleStrategy == ShuffleStrategy::Lazy) lazyDeck_.reset(rng_);
  else fullDeck_.reset(rng_);
  for (unsigned c = 0; c < config_.rules.holeCards; ++c) {
    auto seat = state_.dealerButton;
    for (unsigned n = 0; n < dealt; ++n) {
      seat = nextDealt(seat);
      state_.players[seat].holeCards[c] = draw();
    }
  }
  const auto small = dealt == 2 ? state_.dealerButton : nextDealt(state_.dealerButton);
  const auto big = nextDealt(small);
  for (PlayerId i = 0; i < state_.numPlayers; ++i)
    if (live(state_.players[i].status)) pay(i, config_.ante, false);
  pay(small, config_.smallBlind);
  pay(big, config_.bigBlind);
  // Pot-limit preflop counts nominal blinds even when a blind is short all-in.
  preflopPotCorrection_ = config_.smallBlind - state_.players[small].streetBet +
                         config_.bigBlind - state_.players[big].streetBet;
  state_.currentBet = config_.bigBlind;
  state_.lastFullRaise = config_.bigBlind;
  advance(big);
}

LegalActions PokerEngine::getActions() const noexcept {
  LegalActions a;
  if (state_.status != HandStatus::InProgress || state_.currentPlayer == INVALID_PLAYER) return a;
  const auto& p = state_.players[state_.currentPlayer];
  const Chips owed = std::max<Chips>(0, state_.currentBet - p.streetBet);
  a.toCall = std::min(owed, p.stack);
  a.mask = owed ? CanFold | CanCall : CanCheck;
  bool opponentCanAct = false;
  for (unsigned i = 0; i < state_.numPlayers; ++i)
    opponentCanAct |= i != state_.currentPlayer && state_.players[i].status == PlayerStatus::Active;
  const bool reopened = !p.acted || p.lastActionBet == 0 ||
      state_.currentBet - p.lastActionBet >= state_.lastFullRaise;
  if (opponentCanAct && reopened && p.stack > owed) {
    const Chips allIn = p.streetBet + p.stack;
    Chips maximum = allIn;
    if (config_.rules.betting == BettingStructure::PotLimit) {
      const Chips correction = state_.street == Street::Preflop ? preflopPotCorrection_ : 0;
      maximum = std::min(maximum, p.streetBet + 2 * owed + state_.pot + correction);
    }
    const Chips minimum = state_.currentBet + state_.lastFullRaise;
    if (maximum > state_.currentBet && (maximum >= minimum || maximum == allIn)) {
      a.mask |= CanRaise;
      a.minRaiseTo = std::min(minimum, allIn);
      a.maxRaiseTo = maximum;
    }
  }
  return a;
}

bool PokerEngine::validate(Action action) const noexcept {
  const auto a = getActions();
  switch (action.type) {
    case ActionType::Fold: return action.raise == 0 && a.allows(CanFold);
    case ActionType::Check: return action.raise == 0 && a.allows(CanCheck);
    case ActionType::Call: return action.raise == 0 && a.allows(CanCall);
    case ActionType::Raise:
      return a.allows(CanRaise) && action.raise >= a.minRaiseTo && action.raise <= a.maxRaiseTo;
  }
  return false;
}

StepResult PokerEngine::step(Action action) noexcept {
  if (!validate(action)) return {StepStatus::InvalidAction, state_.currentPlayer};
  const auto id = state_.currentPlayer;
  auto& p = state_.players[id];
  if (action.type == ActionType::Fold) p.status = PlayerStatus::Folded;
  else if (action.type == ActionType::Call) pay(id, state_.currentBet - p.streetBet);
  else if (action.type == ActionType::Raise) {
    const Chips increment = action.raise - state_.currentBet;
    pay(id, action.raise - p.streetBet);
    state_.currentBet = action.raise;
    if (increment >= state_.lastFullRaise) state_.lastFullRaise = increment;
    for (unsigned i = 0; i < state_.numPlayers; ++i) {
      auto& other = state_.players[i];
      if (i != id && other.status == PlayerStatus::Active && other.streetBet < state_.currentBet)
        other.needsAction = true;
    }
  }
  p.needsAction = false;
  p.acted = true;
  p.lastActionBet = state_.currentBet;
  advance(id);
  return {isHandComplete() ? StepStatus::HandComplete : StepStatus::NextDecision, state_.currentPlayer};
}

void PokerEngine::advance(PlayerId from) noexcept {
  for (;;) {
    unsigned remaining = 0, actors = 0;
    PlayerId lone = INVALID_PLAYER;
    Chips actualBet = 0;
    for (PlayerId i = 0; i < state_.numPlayers; ++i) {
      auto& p = state_.players[i];
      remaining += live(p.status);
      actualBet = std::max(actualBet, p.streetBet);
      if (p.status == PlayerStatus::Active) { ++actors; lone = i; }
    }
    if (remaining == 1) { finish(false); return; }
    if (actors == 1) {
      // No dry-side-pot bets. The last actor only owes actual committed chips.
      state_.currentBet = actualBet;
      state_.players[lone].needsAction = state_.players[lone].streetBet < actualBet;
    }
    const auto next = nextDecision(from);
    if (next != INVALID_PLAYER) { state_.currentPlayer = next; return; }
    if (state_.street == Street::River) { finish(true); return; }
    advanceStreet();
    from = state_.dealerButton;
  }
}

void PokerEngine::advanceStreet() noexcept {
  state_.street = static_cast<Street>(static_cast<unsigned>(state_.street) + 1);
  if (config_.rules.burnCards) (void)draw();
  const unsigned count = state_.street == Street::Flop ? 3 : 1;
  for (unsigned n = 0; n < count; ++n) state_.board[state_.boardCount++] = draw();
  state_.currentBet = 0;
  state_.lastFullRaise = config_.bigBlind;
  for (unsigned i = 0; i < state_.numPlayers; ++i) {
    auto& p = state_.players[i];
    p.streetBet = 0;
    p.needsAction = p.status == PlayerStatus::Active;
    p.acted = false;
    p.lastActionBet = 0;
  }
}

void PokerEngine::finish(bool showdown) noexcept {
  if (showdown) {
    state_.street = Street::Showdown;
    std::array<HandRank, MAX_PLAYERS> ranks{};
    for (unsigned i = 0; i < state_.numPlayers; ++i)
      if (live(state_.players[i].status))
        ranks[i] = HandEvaluator::evaluateShowdown(state_.players[i].holeCards, state_.board, config_.rules);
    state_.winnings = PotManager::settle(state_, ranks).winnings;
  } else {
    for (unsigned i = 0; i < state_.numPlayers; ++i)
      if (live(state_.players[i].status)) state_.winnings[i] = state_.pot;
  }
  for (unsigned i = 0; i < state_.numPlayers; ++i) {
    state_.players[i].stack += state_.winnings[i];
    state_.players[i].needsAction = false;
  }
  state_.pot = 0;
  state_.currentPlayer = INVALID_PLAYER;
  state_.status = HandStatus::Complete;
}

Observation PokerEngine::getObservation() const noexcept {
  Observation o;
  o.handNumber = state_.handNumber;
  o.hero = state_.currentPlayer;
  o.button = state_.dealerButton;
  o.street = state_.street;
  o.numPlayers = state_.numPlayers;
  o.holeCardCount = state_.holeCardCount;
  if (o.hero != INVALID_PLAYER) o.holeCards = state_.players[o.hero].holeCards;
  o.board = state_.board;
  o.boardCount = state_.boardCount;
  o.pot = state_.pot;
  o.legal = getActions();
  for (unsigned i = 0; i < state_.numPlayers; ++i) {
    const auto& p = state_.players[i];
    o.stacks[i] = p.stack;
    o.streetBets[i] = p.streetBet;
    o.totalBets[i] = p.totalBet;
    if (live(p.status)) o.activeMask |= static_cast<uint16_t>(1u << i);
    if (p.status == PlayerStatus::AllIn) o.allInMask |= static_cast<uint16_t>(1u << i);
  }
  return o;
}
}
