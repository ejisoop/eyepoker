#include <pokereye/core/engine.h>
#include <pokereye/core/equity.h>
#include <charconv>
#include <chrono>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace PokerEye;
namespace {
uint64_t integer(std::string_view text) {
  uint64_t value = 0;
  const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
  if (result.ec != std::errc{} || result.ptr != text.data() + text.size())
    throw std::invalid_argument("expected a nonnegative integer: " + std::string(text));
  return value;
}
std::vector<Card> cards(std::string_view text) {
  std::string compact;
  for (unsigned char c : text) if (!std::isspace(c) && c != ',') compact += static_cast<char>(c);
  if (compact.size() % 2 != 0) throw std::invalid_argument("cards use pairs such as AsKd (T for ten)");
  std::vector<Card> result;
  for (std::size_t i = 0; i < compact.size(); i += 2) {
    if (compact.substr(i, 2) == "??") { result.push_back(INVALID_CARD); continue; }
    const auto rank = std::string_view("23456789TJQKA").find(static_cast<char>(std::toupper(static_cast<unsigned char>(compact[i]))));
    const auto suit = std::string_view("cdhs").find(static_cast<char>(std::tolower(static_cast<unsigned char>(compact[i + 1]))));
    if (rank == std::string_view::npos || suit == std::string_view::npos) throw std::invalid_argument("invalid card: " + compact.substr(i, 2));
    result.push_back(newCard(static_cast<uint8_t>(rank), static_cast<uint8_t>(suit)));
  }
  return result;
}
GameRules variant(std::string_view name) {
  if (name == "holdem") return GameRules::holdem();
  if (name == "omaha") return GameRules::omaha();
  if (name == "omaha5") return GameRules::omaha(5);
  if (name == "omaha6") return GameRules::omaha(6);
  if (name == "shortdeck") return GameRules::shortDeck();
  throw std::invalid_argument("unknown variant");
}
void help() {
  std::cout << "PokerEye community-card simulator\n\n"
      "poker_sim equity [--variant holdem|omaha|omaha5|omaha6|shortdeck]\n"
      "  [--players 2..12] [--trials 100000] [--threads 1] [--seed 42]\n"
      "  [--hand AsAh] [--hand KsKh] [--board 2c3d4h] [--dead QcQd]\n"
      "  Repeat --hand in seat order; ?? is an unknown card. Omitted seats are random.\n\n"
      "poker_sim play [--variant holdem] [--players 6] [--hands 10000] [--seed 42]\n"
      "  Independent full hands, equal 100-BB stacks, illustrative random legal policy.\n"
      "  Reports throughput, not strategy quality or expected profit.\n";
}
}
int main(int argc, char** argv) {
  try {
    if (argc == 1 || std::string_view(argv[1]) == "--help") { help(); return 0; }
    const std::string_view mode = argv[1];
    if (mode != "equity" && mode != "play") throw std::invalid_argument("mode must be equity or play");
    EquityRequest request;
    if (mode == "play") request.numPlayers = 6;
    uint64_t hands = 10'000;
    std::vector<std::string> knownHands;
    std::string board, dead;
    for (int i = 2; i < argc; ++i) {
      const std::string_view option = argv[i];
      if (option == "--help") { help(); return 0; }
      if (i + 1 == argc) throw std::invalid_argument("missing value for " + std::string(option));
      const std::string_view value = argv[++i];
      if (option == "--variant") request.rules = variant(value);
      else if (option == "--players") {
        const auto n = integer(value);
        if (n < 2 || n > MAX_PLAYERS) throw std::invalid_argument("players must be 2..12");
        request.numPlayers = static_cast<uint8_t>(n);
      } else if (option == "--seed") request.seed = integer(value);
      else if (option == "--hands" && mode == "play") hands = integer(value);
      else if (option == "--trials" && mode == "equity") request.trials = integer(value);
      else if (option == "--threads" && mode == "equity") {
        const auto n = integer(value);
        if (n == 0 || n > 256) throw std::invalid_argument("threads must be 1..256");
        request.threads = static_cast<unsigned>(n);
      } else if (option == "--hand" && mode == "equity") knownHands.emplace_back(value);
      else if (option == "--board" && mode == "equity") board = value;
      else if (option == "--dead" && mode == "equity") dead = value;
      else throw std::invalid_argument("unknown option for this mode: " + std::string(option));
    }
    std::cout << std::fixed << std::setprecision(6);
    if (mode == "equity") {
      if (knownHands.size() > request.numPlayers) throw std::invalid_argument("too many --hand arguments");
      for (std::size_t p = 0; p < knownHands.size(); ++p) {
        const auto parsed = cards(knownHands[p]);
        if (parsed.size() != request.rules.holeCards) throw std::invalid_argument("--hand must contain the variant's full hole-card count (use ?? for unknowns)");
        std::copy(parsed.begin(), parsed.end(), request.holeCards[p].begin());
      }
      const auto parsedBoard = cards(board);
      if (parsedBoard.size() > 5) throw std::invalid_argument("board has more than five cards");
      request.boardCount = static_cast<uint8_t>(parsedBoard.size());
      std::copy(parsedBoard.begin(), parsedBoard.end(), request.board.begin());
      for (auto card : cards(dead)) {
        if (card == INVALID_CARD) throw std::invalid_argument("dead cards cannot be unknown");
        if ((request.deadCards & (uint64_t{1} << card)) != 0) throw std::invalid_argument("duplicate dead card");
        request.deadCards |= uint64_t{1} << card;
      }
      const auto start = std::chrono::steady_clock::now();
      const auto result = simulateEquity(request);
      const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
      std::cout << "seat,equity,standard_error,wins,ties\n";
      for (unsigned p = 0; p < result.numPlayers; ++p) {
        const auto& player = result.players[p];
        std::cout << p << ',' << player.equity << ',' << player.standardError << ',' << player.wins << ',' << player.ties << '\n';
      }
      std::cout << "trials=" << result.trials << " seconds=" << seconds
                << " trials_per_second=" << static_cast<double>(result.trials) / seconds << '\n';
    } else {
      if (hands == 0 || hands > 1'000'000'000) throw std::invalid_argument("hands must be 1..1e9");
      GameConfig config; config.rules = request.rules; config.numPlayers = request.numPlayers;
      const PokerEngine valid(config); // Validate before timing.
      (void)valid;
      Random seedSource(request.seed), policy(request.seed ^ 0xabcdef1234567890ULL);
      uint64_t decisions = 0;
      std::array<Chips, MAX_PLAYERS> net{};
      const auto start = std::chrono::steady_clock::now();
      for (uint64_t hand = 0; hand < hands; ++hand) {
        config.seed = seedSource.next();
        PokerEngine engine(config); engine.startHand();
        while (!engine.isHandComplete()) {
          ++decisions;
          const auto legal = engine.getActions();
          const unsigned pick = policy.bounded32(8);
          Action action{legal.allows(CanCheck) ? ActionType::Check : ActionType::Call};
          if (pick == 0 && legal.allows(CanFold)) action = {ActionType::Fold};
          else if (pick <= 2 && legal.allows(CanRaise)) action = {ActionType::Raise, legal.minRaiseTo};
          if (engine.step(action).status == StepStatus::InvalidAction) throw std::logic_error("policy produced an invalid action");
        }
        for (unsigned p = 0; p < config.numPlayers; ++p) net[p] += engine.getState().players[p].stack - config.startingStack;
      }
      const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
      std::cout << "hands=" << hands << " decisions=" << decisions << " seconds=" << seconds
                << " hands_per_second=" << static_cast<double>(hands) / seconds << '\n';
      for (unsigned p = 0; p < config.numPlayers; ++p) std::cout << "seat=" << p << " net_chips=" << net[p] << '\n';
    }
  } catch (const std::exception& e) { std::cerr << "error: " << e.what() << '\n'; return 1; }
}
