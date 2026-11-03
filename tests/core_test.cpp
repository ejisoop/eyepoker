#include "reference.h"
#include <pokereye/core/engine.h>
#include <pokereye/core/equity.h>
#include <pokereye/core/obervation.h>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace PokerEye;
#define CHECK(x) do { if (!(x)) throw std::runtime_error(std::string(#x) + " at line " + std::to_string(__LINE__)); } while (false)
template<class F> void rejects(F&& f) {
  bool rejected = false;
  try { f(); } catch (const std::invalid_argument&) { rejected = true; }
  CHECK(rejected);
}
void act(PokerEngine& e, ActionType type, Chips raise = 0) {
  CHECK(e.step({type, raise}).status != StepStatus::InvalidAction);
}
void callOrCheck(PokerEngine& e) {
  act(e, e.getActions().allows(CanCheck) ? ActionType::Check : ActionType::Call);
}
void finishPassive(PokerEngine& e) {
  unsigned n = 0;
  while (!e.isHandComplete()) { CHECK(++n < 200); callOrCheck(e); }
}
void randomAndDeck() {
  Random a(42), b(42), c(43);
  bool differs = false;
  for (unsigned n = 0; n < 10'000; ++n) {
    const auto v = a.next(); CHECK(v == b.next()); differs |= v != c.next();
  }
  CHECK(differs);
  a.reseed(42); b.reseed(42);
  for (unsigned n = 0; n < 100; ++n) CHECK(a.next() == b.next());
  for (uint32_t bound : {1u, 2u, 3u, 36u, 52u, 65537u, 2147483649u, 4294967295u})
    for (unsigned n = 0; n < 10'000; ++n) {
      CHECK(a.bounded32(bound) < bound); CHECK(a.bounded(bound) < bound);
    }
  std::array<unsigned, 13> counts{};
  for (unsigned i = 0; i < 130'000; ++i) ++counts[a.bounded32(13)];
  for (auto count : counts) CHECK(count > 9400 && count < 10600);
  auto testDeck = []<class D>(D deck, unsigned minimum) {
    Random rng(42);
    std::array<Card, 52> first{};
    for (unsigned cycle = 0; cycle < 50; ++cycle) {
      deck.reset(rng);
      uint64_t seen = 0;
      const auto n = deck.remaining();
      CHECK(n == (13 - minimum) * 4);
      for (unsigned i = 0; i < n; ++i) {
        const auto card = deck.draw(rng);
        if (cycle == 0) first[i] = card;
        CHECK(card >= minimum * 4 && card < 52);
        CHECK((seen & (uint64_t{1} << card)) == 0);
        seen |= uint64_t{1} << card;
      }
      CHECK(deck.remaining() == 0);
    }
    deck.restore(); rng.reseed(42); deck.reset(rng);
    for (unsigned i = 0; i < (13 - minimum) * 4; ++i) CHECK(deck.draw(rng) == first[i]);
  };
  testDeck(Deck<ShuffleStrategy::Lazy>{}, 0);
  testDeck(Deck<ShuffleStrategy::Full>{}, 0);
  testDeck(Deck<ShuffleStrategy::Lazy>{4}, 4);
  testDeck(Deck<ShuffleStrategy::Full>{4}, 4);
  rejects([] { Deck<> invalid(9); });
}
void exhaustiveFive() {
  std::array<unsigned, 9> counts{};
  uint64_t hands = 0;
  for (Card a = 0; a < 48; ++a)
    for (Card b = a + 1; b < 49; ++b)
      for (Card c = b + 1; c < 50; ++c)
        for (Card d = c + 1; d < 51; ++d)
          for (Card e = d + 1; e < 52; ++e) {
            const std::array<Card, 5> cards{a,b,c,d,e};
            const auto actual = HandEvaluator::evaluateFive(cards);
            CHECK(actual == referenceFive(cards));
            ++counts[actual >> 20]; ++hands;
          }
  CHECK(hands == 2598960);
  CHECK((counts == std::array<unsigned, 9>{1302540,1098240,123552,54912,10200,5108,3744,624,40}));
}
void shortDeckFive() {
  auto rules = GameRules::shortDeck();
  for (Card a = 16; a < 48; ++a)
    for (Card b = a + 1; b < 49; ++b)
      for (Card c = b + 1; c < 50; ++c)
        for (Card d = c + 1; d < 51; ++d)
          for (Card e = d + 1; e < 52; ++e) {
            const std::array<Card, 5> cards{a,b,c,d,e};
            CHECK(HandEvaluator::evaluateFive(cards, rules) == referenceFive(cards, rules));
          }
  const std::array<Card, 5> low{newCard(12,0),newCard(4,1),newCard(5,2),newCard(6,3),newCard(7,0)};
  CHECK(HandEvaluator::evaluateFive(low, rules) == ((4u << 20) | (7u << 16)));
}
void sevenAndVariants() {
  Random rng(718);
  for (auto rules : {GameRules::holdem(), GameRules::shortDeck()}) {
    Deck<> deck(rules.minimumRank);
    for (unsigned trial = 0; trial < 100'000; ++trial) {
      deck.reset(rng);
      std::array<Card, 7> seven{};
      for (auto& card : seven) card = deck.draw(rng);
      std::array<Card, MAX_HOLE_CARDS> hole{};
      hole[0] = seven[0]; hole[1] = seven[1];
      std::array<Card, 5> board{};
      std::copy_n(seven.begin() + 2, 5, board.begin());
      CHECK(HandEvaluator::evaluateShowdown(hole, board, rules) == referenceBest(seven.data(), 7, rules));
      if (rules.minimumRank == 0) CHECK(HandEvaluator::evaluateSeven(seven) == referenceBest(seven.data(), 7));
      if (trial < 1000) {
        rules.tripsBeatStraight = true;
        CHECK(HandEvaluator::evaluateShowdown(hole, board, rules) == referenceBest(seven.data(), 7, rules));
        rules.tripsBeatStraight = false;
      }
    }
  }
  Deck<> deck;
  for (unsigned holes = 2; holes <= MAX_HOLE_CARDS; ++holes) {
    auto rules = GameRules::omaha(static_cast<uint8_t>(holes));
    for (unsigned t = 0; t < 1000; ++t) {
      deck.reset(rng);
      std::array<Card, MAX_HOLE_CARDS> hole{};
      std::array<Card, 5> board{};
      std::array<Card, MAX_HOLE_CARDS + 5> all{};
      for (unsigned i = 0; i < holes; ++i) all[i] = hole[i] = deck.draw(rng);
      for (unsigned i = 0; i < 5; ++i) all[holes + i] = board[i] = deck.draw(rng);
      HandRank expected = 0;
      // Independent oracle selects every five-card subset, then filters hole count.
      const unsigned n = holes + 5;
      for (unsigned a = 0; a + 4 < n; ++a)
        for (unsigned b = a + 1; b + 3 < n; ++b)
          for (unsigned c = b + 1; c + 2 < n; ++c)
            for (unsigned d = c + 1; d + 1 < n; ++d)
              for (unsigned e = d + 1; e < n; ++e)
                if ((a < holes) + (b < holes) + (c < holes) + (d < holes) + (e < holes) == 2)
                  expected = std::max(expected, referenceFive({all[a],all[b],all[c],all[d],all[e]}));
      CHECK(HandEvaluator::evaluateShowdown(hole, board, rules) == expected);
      rules.selection = HandSelection::AnyFive;
      CHECK(HandEvaluator::evaluateShowdown(hole, board, rules) == referenceBest(all.data(), n));
      rules.selection = HandSelection::ExactlyTwo;
    }
  }
  const std::array<Card, 5> royal{32,36,40,44,48};
  const std::array<Card, MAX_HOLE_CARDS> low{1,6,11,13,INVALID_CARD,INVALID_CARD};
  CHECK((HandEvaluator::evaluateShowdown(low, royal, GameRules::holdem()) >> 20) == 8);
  CHECK((HandEvaluator::evaluateShowdown(low, royal, GameRules::omaha()) >> 20) != 8);
}
void headsUpAndInvalid() {
  GameConfig config; config.numPlayers = 2;
  PokerEngine e(config); e.startHand();
  CHECK(e.getState().dealerButton == 0 && e.getState().currentPlayer == 0);
  CHECK(e.getActions().toCall == 50 && e.getActions().minRaiseTo == 200);
  const auto copy = e;
  CHECK(e.step({ActionType::Check}).status == StepStatus::InvalidAction);
  CHECK(e.step({ActionType::Raise, 199}).status == StepStatus::InvalidAction);
  CHECK(e.step({ActionType::Raise, -1}).status == StepStatus::InvalidAction);
  CHECK(e.step({static_cast<ActionType>(255)}).status == StepStatus::InvalidAction);
  CHECK(e.getState().pot == copy.getState().pot && e.getState().currentPlayer == copy.getState().currentPlayer);
  auto cloned = copy;
  callOrCheck(e); callOrCheck(cloned);
  CHECK(e.getActions().allows(CanCheck)); // BB retains its option.
  callOrCheck(e); callOrCheck(cloned);
  CHECK(e.getState().street == Street::Flop && e.getState().currentPlayer == 1);
  CHECK(e.getState().board == cloned.getState().board);
  finishPassive(e); finishPassive(cloned);
  CHECK(e.getState().winnings == cloned.getState().winnings);
  CHECK(e.getActions().mask == 0 && e.getObservation().hero == INVALID_PLAYER);
  CHECK(e.step({ActionType::Call}).status == StepStatus::InvalidAction);
  e.startHand();
  CHECK(e.getState().dealerButton == 1 && e.getState().currentPlayer == 1);
  bool threw = false;
  try { e.startHand(); } catch (const std::logic_error&) { threw = true; }
  CHECK(threw);
}
void foldAndAllIn() {
  GameConfig config; config.numPlayers = 2;
  PokerEngine e(config); e.startHand(); act(e, ActionType::Fold);
  CHECK(e.isHandComplete() && e.getState().boardCount == 0);
  CHECK(e.getState().players[0].stack == 9950 && e.getState().players[1].stack == 10050);
  config.initialStacks[0] = 1000; config.initialStacks[1] = 75;
  PokerEngine shortBlind(config); shortBlind.startHand();
  CHECK(shortBlind.getActions().toCall == 25);
  CHECK(!shortBlind.getActions().allows(CanRaise));
  callOrCheck(shortBlind);
  CHECK(shortBlind.isHandComplete() && shortBlind.getState().boardCount == 5);
  CHECK(shortBlind.getState().players[0].stack + shortBlind.getState().players[1].stack == 1075);
  config.ante = 1000;
  PokerEngine antes(config); antes.startHand();
  CHECK(antes.isHandComplete() && antes.getState().boardCount == 5);
  CHECK(antes.getState().players[0].stack + antes.getState().players[1].stack == 1075);
}
void raiseReopening() {
  GameConfig c; c.numPlayers = 3; c.initialStacks = {1000,150,1000};
  PokerEngine e(c); e.startHand();
  act(e, ActionType::Call); // Button has acted at 100.
  CHECK(e.getActions().minRaiseTo == 150 && e.getActions().maxRaiseTo == 150);
  act(e, ActionType::Raise, 150); // Short all-in does not reopen button.
  act(e, ActionType::Call);
  CHECK(e.getState().currentPlayer == 0 && !e.getActions().allows(CanRaise));
  callOrCheck(e); finishPassive(e);
  c.numPlayers = 4; c.initialStacks = {150,200,1000,1000};
  PokerEngine cumulative(c); cumulative.startHand();
  CHECK(cumulative.getState().currentPlayer == 3);
  act(cumulative, ActionType::Call);
  act(cumulative, ActionType::Raise, 150);
  act(cumulative, ActionType::Raise, 200);
  act(cumulative, ActionType::Call);
  CHECK(cumulative.getState().currentPlayer == 3 && cumulative.getActions().allows(CanRaise));
  CHECK(cumulative.getActions().minRaiseTo == 300);
  act(cumulative, ActionType::Raise, 300);
  finishPassive(cumulative);
  // A check does not prevent raising an opening short all-in on a new street.
  c.numPlayers = 3; c.initialStacks = {1000,1000,150};
  PokerEngine checked(c); checked.startHand();
  callOrCheck(checked); callOrCheck(checked); callOrCheck(checked);
  act(checked, ActionType::Check); // SB checks flop.
  act(checked, ActionType::Raise, 50); // BB short opener.
  act(checked, ActionType::Call);
  CHECK(checked.getState().currentPlayer == 1 && checked.getActions().allows(CanRaise));
  CHECK(checked.getActions().minRaiseTo == 150);
}
void potLimit() {
  GameConfig c; c.numPlayers = 3; c.rules = GameRules::omaha();
  PokerEngine e(c); e.startHand();
  CHECK(e.getActions().maxRaiseTo == 350);
  CHECK(e.step({ActionType::Raise, 351}).status == StepStatus::InvalidAction);
  act(e, ActionType::Raise, 350);
  CHECK(e.getActions().maxRaiseTo == 1150);
  finishPassive(e);
  c.initialStacks = {1000,1000,60};
  PokerEngine shortBlind(c); shortBlind.startHand();
  CHECK(shortBlind.getActions().maxRaiseTo == 350);
  c.initialStacks = {}; c.ante = 25;
  PokerEngine antes(c); antes.startHand();
  CHECK(antes.getActions().maxRaiseTo == 425);
}
void sidePots() {
  GameState s; s.numPlayers = 4; s.dealerButton = 0;
  const std::array<Chips, 4> bets{50,100,200,200};
  for (unsigned i = 0; i < 4; ++i) { s.players[i].totalBet = bets[i]; s.players[i].status = PlayerStatus::AllIn; }
  std::array<HandRank, MAX_PLAYERS> ranks{9,7,8,6};
  auto result = PotManager::settle(s, ranks);
  CHECK(result.winnings[0] == 200 && result.winnings[2] == 350);
  s.players[2].status = PlayerStatus::Folded;
  result = PotManager::settle(s, ranks);
  CHECK(result.winnings[0] == 200 && result.winnings[1] == 150 && result.winnings[3] == 200);
  s = {}; s.numPlayers = 3; s.dealerButton = 0;
  for (unsigned i = 0; i < 3; ++i) { s.players[i].totalBet = 5; s.players[i].status = PlayerStatus::AllIn; }
  s.players[2].status = PlayerStatus::Folded; ranks = {10,10,100};
  result = PotManager::settle(s, ranks);
  CHECK(result.winnings[0] == 7 && result.winnings[1] == 8 && result.winnings[2] == 0);
  s.players[0].totalBet = 100; ranks = {1,10,100};
  result = PotManager::settle(s, ranks);
  CHECK(result.winnings[0] == 95 && result.winnings[1] == 15);
}
void configValidation() {
  GameConfig c;
  c.numPlayers = 1; rejects([&] { PokerEngine e(c); });
  c = {}; c.bigBlind = 0; rejects([&] { PokerEngine e(c); });
  c = {}; c.rules = GameRules::omaha(6); c.numPlayers = 12; rejects([&] { PokerEngine e(c); });
  c = {}; c.rules.minimumRank = 9; rejects([&] { PokerEngine e(c); });
  c = {}; c.initialStacks[0] = -1; rejects([&] { PokerEngine e(c); });
  c = {}; c.startingStack = std::numeric_limits<Chips>::max(); rejects([&] { PokerEngine e(c); });
  c = {}; c.initialStacks = {100,0,100};
  PokerEngine e(c); e.startHand(); CHECK(e.getState().players[1].status == PlayerStatus::SittingOut);
  CHECK(e.getState().currentPlayer == 0); finishPassive(e);
}
void randomGameInvariants() {
  Random policy(0x12345);
  uint64_t decisions = 0;
  for (auto rules : {GameRules::holdem(),GameRules::omaha(),GameRules::omaha(5),GameRules::shortDeck()})
    for (unsigned trial = 0; trial < 2000; ++trial) {
      GameConfig c; c.rules = rules; c.seed = policy.next();
      c.numPlayers = static_cast<uint8_t>(2 + policy.bounded32(5));
      c.shuffleStrategy = trial % 2 ? ShuffleStrategy::Lazy : ShuffleStrategy::Full;
      c.ante = policy.bounded32(20);
      Chips initial = 0;
      for (unsigned i = 0; i < c.numPlayers; ++i) initial += c.initialStacks[i] = 1 + policy.bounded32(2000);
      PokerEngine e(c); e.startHand(); unsigned steps = 0;
      while (!e.isHandComplete()) {
        CHECK(++steps < 1000); ++decisions;
        const auto& s = e.getState(); Chips total = s.pot; uint64_t seen = 0;
        auto card = [&](Card value) {
          CHECK(value < 52 && value >= rules.minimumRank * 4);
          CHECK((seen & (uint64_t{1} << value)) == 0); seen |= uint64_t{1} << value;
        };
        for (unsigned i = 0; i < c.numPlayers; ++i) {
          const auto& p = s.players[i]; total += p.stack;
          CHECK(p.stack >= 0 && p.streetBet >= 0 && p.totalBet >= p.streetBet);
          for (unsigned h = 0; h < rules.holeCards; ++h) card(p.holeCards[h]);
        }
        for (unsigned i = 0; i < s.boardCount; ++i) card(s.board[i]);
        CHECK(total == initial);
        CHECK(s.currentPlayer < s.numPlayers && s.players[s.currentPlayer].status == PlayerStatus::Active);
        const auto obs = e.getObservation(); const auto legal = obs.legal;
        CHECK(obs.hero == s.currentPlayer && obs.holeCards == s.players[s.currentPlayer].holeCards);
        const unsigned choice = policy.bounded32(5);
        if (choice == 0 && legal.allows(CanFold)) act(e, ActionType::Fold);
        else if (choice <= 2 && legal.allows(CanRaise)) {
          const auto target = legal.minRaiseTo + static_cast<Chips>(policy.bounded(
              static_cast<uint64_t>(legal.maxRaiseTo - legal.minRaiseTo + 1)));
          act(e, ActionType::Raise, target);
        } else callOrCheck(e);
      }
      Chips total = 0, contributed = 0, awarded = 0;
      for (unsigned i = 0; i < c.numPlayers; ++i) {
        const auto& p = e.getState().players[i]; total += p.stack; contributed += p.totalBet;
        awarded += e.getState().winnings[i]; CHECK(p.stack >= 0);
      }
      CHECK(total == initial && contributed == awarded && e.getState().pot == 0);
    }
  CHECK(decisions > 10'000);
}
void equityKnownAndValidation() {
  EquityRequest r; r.trials = 100;
  r.board = {32,36,40,44,48}; r.boardCount = 5;
  auto result = simulateEquity(r);
  CHECK(result.players[0].equity == 0.5 && result.players[1].equity == 0.5);
  CHECK(result.players[0].ties == 100 && result.players[0].standardError == 0);
  r.trials = 1;
  CHECK(std::isnan(simulateEquity(r).players[0].standardError));
  r.trials = 100;
  r.board = {0,5,10,19,24};
  r.holeCards[0][0] = 48; r.holeCards[0][1] = 49;
  r.holeCards[1][0] = 44; r.holeCards[1][1] = 45;
  result = simulateEquity(r);
  CHECK(result.players[0].equity == 1 && result.players[0].wins == 100);
  r.holeCards[1][1] = 48; rejects([&] { (void)simulateEquity(r); });
  r = {}; r.deadCards = uint64_t{1} << 60; rejects([&] { (void)simulateEquity(r); });
  r = {}; r.rules = GameRules::shortDeck(); r.holeCards[0][0] = 0; rejects([&] { (void)simulateEquity(r); });
  r = {}; r.threads = 0; rejects([&] { (void)simulateEquity(r); });
  r = {}; r.trials = 0; rejects([&] { (void)simulateEquity(r); });
  r = {}; r.holeCards[4][0] = 1; rejects([&] { (void)simulateEquity(r); });
  r = {}; r.board[4] = 1; rejects([&] { (void)simulateEquity(r); });
  r = {}; r.deadCards = (uint64_t{1} << 52) - 1; rejects([&] { (void)simulateEquity(r); });
}
void equityParallelAndAccuracy() {
  EquityRequest r; r.trials = 25'000; r.numPlayers = 4;
  r.holeCards[0][0] = 48; r.holeCards[0][1] = 49;
  const auto one = simulateEquity(r); r.threads = 4; const auto four = simulateEquity(r);
  double sum = 0;
  for (unsigned p = 0; p < r.numPlayers; ++p) {
    CHECK(one.players[p].wins == four.players[p].wins && one.players[p].ties == four.players[p].ties);
    CHECK(one.players[p].equity == four.players[p].equity);
    CHECK(one.players[p].standardError == four.players[p].standardError);
    sum += one.players[p].equity;
  }
  CHECK(std::abs(sum - 1) < 1e-12);
  // Compare sampled river equity against exact enumeration of all legal rivers.
  r = {}; r.trials = 100'000;
  r.holeCards[0][0] = 48; r.holeCards[0][1] = 49;
  r.holeCards[1][0] = 44; r.holeCards[1][1] = 45;
  r.board = {0,5,10,19,INVALID_CARD}; r.boardCount = 4;
  unsigned wins = 0, ties = 0, rivers = 0;
  for (Card river = 0; river < 52; ++river) {
    if (river == 48 || river == 49 || river == 44 || river == 45 || river == 0 || river == 5 || river == 10 || river == 19) continue;
    auto board = r.board; board[4] = river;
    const std::array<Card, 7> a{48,49,board[0],board[1],board[2],board[3],river};
    const std::array<Card, 7> b{44,45,board[0],board[1],board[2],board[3],river};
    const auto first = referenceBest(a.data(),7), second = referenceBest(b.data(),7);
    wins += first > second; ties += first == second; ++rivers;
  }
  const double exact = (wins + 0.5 * ties) / rivers;
  auto sampled = simulateEquity(r);
  CHECK(std::abs(sampled.players[0].equity - exact) < 0.008);
  // All unknown three-way players must be symmetric within sampling error.
  r = {}; r.numPlayers = 3; r.trials = 100'000; sampled = simulateEquity(r);
  for (unsigned p = 0; p < 3; ++p) CHECK(std::abs(sampled.players[p].equity - 1.0/3) < 0.01);
}
int main(int argc, char** argv) {
  const bool quick = argc > 1 && std::strcmp(argv[1], "--quick") == 0;
  unsigned passed = 0;
  auto run = [&](const char* name, auto test) { test(); ++passed; std::cout << "PASS " << name << '\n'; };
  try {
    run("random and deck", randomAndDeck);
    if (!quick) {
      run("all 2,598,960 five-card hands", exhaustiveFive);
      run("all 376,992 short-deck five-card hands", shortDeckFive);
      run("200,000 seven-card hands and 5,000 variant hands", sevenAndVariants);
    }
    run("heads-up, cloning and invalid actions", headsUpAndInvalid);
    run("folds, short blinds, all-in antes", foldAndAllIn);
    run("short raises and cumulative reopening", raiseReopening);
    run("pot-limit sizing", potLimit);
    run("side pots, odd chips, uncalled bets", sidePots);
    run("configuration validation", configValidation);
    run("8,000 randomized complete games", randomGameInvariants);
    run("known equity and input validation", equityKnownAndValidation);
    run("parallel determinism and exact equity oracle", equityParallelAndAccuracy);
    std::cout << passed << " test groups passed\n";
  } catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
