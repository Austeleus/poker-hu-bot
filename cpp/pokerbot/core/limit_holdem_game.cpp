#include "limit_holdem_game.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>

#include "hand_evaluator.h"

namespace pokerbot::core {
namespace {

constexpr int Opponent(int player) { return 1 - player; }

}  // namespace

GameState::GameState(GameConfig config) : config_(config) {
  Reset(0);
}

void GameState::Reset(uint64_t seed) {
  std::iota(deck_.begin(), deck_.end(), 0);
  std::mt19937_64 rng(seed);
  std::shuffle(deck_.begin(), deck_.end(), rng);
  InitializeHand();
}

void GameState::ResetWithDeck(const std::array<uint8_t, kDeckSize>& deck) {
  deck_ = deck;
  InitializeHand();
}

void GameState::InitializeHand() {
  deck_position_ = 0;
  for (int player = 0; player < kNumPlayers; ++player) {
    hole_cards_[player][0] = deck_[deck_position_++];
  }
  for (int player = 0; player < kNumPlayers; ++player) {
    hole_cards_[player][1] = deck_[deck_position_++];
  }
  for (int i = 0; i < 5; ++i) {
    board_cards_[i] = deck_[deck_position_++];
  }
  board_count_ = 0;

  betting_round_ = 0;
  current_player_ = 0;
  round_first_player_ = current_player_;

  total_contribution_.fill(0);
  round_contribution_.fill(0);
  total_contribution_[0] = config_.small_blind;
  total_contribution_[1] = config_.big_blind;
  round_contribution_[0] = config_.small_blind;
  round_contribution_[1] = config_.big_blind;
  pot_ = total_contribution_[0] + total_contribution_[1];
  current_bet_ = config_.big_blind;
  raises_in_round_ = 0;
  bet_made_in_round_ = true;

  terminal_ = false;
  terminal_reason_ = TerminalReason::kNone;
  winner_ = -1;
  payoffs_.fill(0);
  action_history_.clear();
}

int64_t GameState::ToCall(int player) const {
  if (player < 0 || player >= kNumPlayers) {
    throw std::out_of_range("Invalid player index");
  }
  const int64_t to_call = current_bet_ - round_contribution_[player];
  return std::max<int64_t>(0, to_call);
}

int64_t GameState::total_contribution(int player) const {
  if (player < 0 || player >= kNumPlayers) {
    throw std::out_of_range("Invalid player index");
  }
  return total_contribution_[player];
}

int64_t GameState::round_contribution(int player) const {
  if (player < 0 || player >= kNumPlayers) {
    throw std::out_of_range("Invalid player index");
  }
  return round_contribution_[player];
}

const std::array<uint8_t, 2>& GameState::hole_cards(int player) const {
  if (player < 0 || player >= kNumPlayers) {
    throw std::out_of_range("Invalid player index");
  }
  return hole_cards_[player];
}

std::vector<uint8_t> GameState::board_cards() const {
  return std::vector<uint8_t>(board_cards_.begin(),
                              board_cards_.begin() + board_count_);
}

std::vector<ActionType> GameState::LegalActions() const {
  if (terminal_) {
    return {};
  }

  const int player = current_player_;
  const int64_t to_call = ToCall(player);
  std::vector<ActionType> actions;
  actions.reserve(3);
  const bool raise_available =
      bet_made_in_round_ && (raises_in_round_ < config_.max_raises_per_round);

  if (to_call > 0) {
    actions.push_back(ActionType::kFold);
    actions.push_back(ActionType::kCall);
    if (raise_available) {
      actions.push_back(ActionType::kRaise);
    }
  } else {
    actions.push_back(ActionType::kCheck);
    if (!bet_made_in_round_) {
      actions.push_back(ActionType::kBet);
    } else if (raise_available) {
      actions.push_back(ActionType::kRaise);
    }
  }

  return actions;
}

bool GameState::CanRaise() const {
  if (!bet_made_in_round_) {
    return true;
  }
  return raises_in_round_ < config_.max_raises_per_round;
}

int64_t GameState::BetSizeForCurrentRound() const {
  if (betting_round_ <= 1) {
    return config_.small_bet;
  }
  return config_.big_bet;
}

bool GameState::ApplyAction(ActionType action) {
  if (terminal_) {
    return false;
  }

  const auto legal = LegalActions();
  if (std::find(legal.begin(), legal.end(), action) == legal.end()) {
    return false;
  }

  const int player = current_player_;
  const int opponent = Opponent(player);
  bool round_complete = false;

  switch (action) {
    case ActionType::kFold: {
      ResolveFold(player);
      break;
    }
    case ActionType::kCheck: {
      if (current_bet_ != 0) {
        return false;
      }
      round_complete = (opponent == round_first_player_);
      break;
    }
    case ActionType::kCall: {
      const int64_t to_call = ToCall(player);
      const int64_t contribution = to_call;
      round_contribution_[player] += contribution;
      total_contribution_[player] += contribution;
      pot_ += contribution;
      round_complete = true;
      break;
    }
    case ActionType::kBet: {
      if (bet_made_in_round_) {
        return false;
      }
      const int64_t bet = BetSizeForCurrentRound();
      current_bet_ = bet;
      round_contribution_[player] += bet;
      total_contribution_[player] += bet;
      pot_ += bet;
      bet_made_in_round_ = true;
      break;
    }
    case ActionType::kRaise: {
      if (!CanRaise()) {
        return false;
      }
      const int64_t raise_amount = BetSizeForCurrentRound();
      const int64_t new_bet = current_bet_ + raise_amount;
      const int64_t delta = new_bet - round_contribution_[player];
      round_contribution_[player] += delta;
      total_contribution_[player] += delta;
      pot_ += delta;
      current_bet_ = new_bet;
      bet_made_in_round_ = true;
      ++raises_in_round_;
      break;
    }
  }

  action_history_.push_back(
      ActionLogEntry{player, betting_round_, action});

  if (terminal_) {
    return true;
  }

  if (round_complete) {
    AdvanceRound();
  } else {
    current_player_ = opponent;
  }

  return true;
}

void GameState::AdvanceRound() {
  round_contribution_.fill(0);
  current_bet_ = 0;
  raises_in_round_ = 0;
  bet_made_in_round_ = false;

  ++betting_round_;

  if (betting_round_ == 1) {
    board_count_ = 3;
  } else if (betting_round_ == 2) {
    board_count_ = 4;
  } else if (betting_round_ == 3) {
    board_count_ = 5;
  } else {
    ResolveShowdown();
    return;
  }

  current_player_ = 1;
  round_first_player_ = current_player_;
}

void GameState::ResolveFold(int folding_player) {
  terminal_ = true;
  terminal_reason_ = TerminalReason::kFold;
  winner_ = Opponent(folding_player);
  payoffs_[folding_player] = -total_contribution_[folding_player];
  payoffs_[winner_] = pot_ - total_contribution_[winner_];
  current_player_ = -1;
}

void GameState::ResolveShowdown() {
  terminal_ = true;
  terminal_reason_ = TerminalReason::kShowdown;
  board_count_ = 5;

  std::vector<uint8_t> board(board_cards_.begin(),
                             board_cards_.begin() + board_count_);

  std::vector<uint8_t> hand0 = board;
  hand0.push_back(hole_cards_[0][0]);
  hand0.push_back(hole_cards_[0][1]);

  std::vector<uint8_t> hand1 = board;
  hand1.push_back(hole_cards_[1][0]);
  hand1.push_back(hole_cards_[1][1]);

  const int cmp = CompareHands(hand0, hand1);
  if (cmp > 0) {
    winner_ = 0;
    payoffs_[0] = pot_ - total_contribution_[0];
    payoffs_[1] = -total_contribution_[1];
  } else if (cmp < 0) {
    winner_ = 1;
    payoffs_[1] = pot_ - total_contribution_[1];
    payoffs_[0] = -total_contribution_[0];
  } else {
    winner_ = -1;
    const int64_t half = pot_ / 2;
    const int64_t remainder = pot_ % 2;
    payoffs_[0] = half + remainder - total_contribution_[0];
    payoffs_[1] = half - total_contribution_[1];
  }

  current_player_ = -1;
}

}  // namespace pokerbot::core
