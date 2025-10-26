#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <random>
#include <vector>

#include "cards.h"

namespace pokerbot::core {

constexpr int kNumPlayers = 2;

enum class ActionType : int {
  kFold = 0,
  kCheck = 1,
  kCall = 2,
  kBet = 3,
  kRaise = 4,
};

enum class TerminalReason : int {
  kNone = 0,
  kFold = 1,
  kShowdown = 2,
};

struct GameConfig {
  int small_blind = 1;
  int big_blind = 2;
  int small_bet = 2;
  int big_bet = 4;
  int max_raises_per_round = 3;
};

struct ActionLogEntry {
  int player = -1;
  int betting_round = -1;
  ActionType action = ActionType::kFold;
};

class GameState {
 public:
  explicit GameState(GameConfig config = GameConfig());

  void Reset(uint64_t seed);

  // Provides a deterministic reset using a predefined deck ordering.
  void ResetWithDeck(const std::array<uint8_t, kDeckSize>& deck);

  const GameConfig& config() const { return config_; }

  int current_player() const { return current_player_; }
  int betting_round() const { return betting_round_; }
  bool is_terminal() const { return terminal_; }
  TerminalReason terminal_reason() const { return terminal_reason_; }
  int winner() const { return winner_; }

  int raises_in_round() const { return raises_in_round_; }
  bool bet_made_in_round() const { return bet_made_in_round_; }

  int64_t pot() const { return pot_; }
  int64_t current_bet() const { return current_bet_; }
  int64_t ToCall(int player) const;
  int64_t total_contribution(int player) const;
  int64_t round_contribution(int player) const;

  const std::array<uint8_t, 2>& hole_cards(int player) const;
  std::vector<uint8_t> board_cards() const;
  int board_card_count() const { return board_count_; }

  const std::vector<ActionLogEntry>& action_history() const {
    return action_history_;
  }

  std::array<int64_t, kNumPlayers> payoffs() const { return payoffs_; }

  std::vector<ActionType> LegalActions() const;
  bool ApplyAction(ActionType action);

 private:
  void InitializeHand();
  void AdvanceRound();
  void ResolveFold(int folding_player);
  void ResolveShowdown();
  int64_t BetSizeForCurrentRound() const;
  bool CanRaise() const;

  GameConfig config_;
  std::array<uint8_t, kDeckSize> deck_{};
  size_t deck_position_ = 0;

  std::array<std::array<uint8_t, 2>, kNumPlayers> hole_cards_{};
  std::array<uint8_t, 5> board_cards_{};
  int board_count_ = 0;

  int betting_round_ = 0;
  int current_player_ = 0;
  int round_first_player_ = 0;

  std::array<int64_t, kNumPlayers> total_contribution_{};
  std::array<int64_t, kNumPlayers> round_contribution_{};
  int64_t pot_ = 0;
  int64_t current_bet_ = 0;
  int raises_in_round_ = 0;
  bool bet_made_in_round_ = false;

  bool terminal_ = false;
  TerminalReason terminal_reason_ = TerminalReason::kNone;
  int winner_ = -1;  // -1 indicates a tie
  std::array<int64_t, kNumPlayers> payoffs_{};

  std::vector<ActionLogEntry> action_history_;
};

}  // namespace pokerbot::core
