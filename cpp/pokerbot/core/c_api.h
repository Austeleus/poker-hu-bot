#pragma once

#include <cstdint>

#include "limit_holdem_game.h"

extern "C" {

struct PokerbotGameState;

enum PokerbotAction : int {
  POKERBOT_ACTION_FOLD = static_cast<int>(pokerbot::core::ActionType::kFold),
  POKERBOT_ACTION_CHECK = static_cast<int>(pokerbot::core::ActionType::kCheck),
  POKERBOT_ACTION_CALL = static_cast<int>(pokerbot::core::ActionType::kCall),
  POKERBOT_ACTION_BET = static_cast<int>(pokerbot::core::ActionType::kBet),
  POKERBOT_ACTION_RAISE = static_cast<int>(pokerbot::core::ActionType::kRaise),
};

PokerbotGameState* pokerbot_state_create();
void pokerbot_state_destroy(PokerbotGameState* state);

void pokerbot_state_reset(PokerbotGameState* state, uint64_t seed);
void pokerbot_state_reset_with_deck(PokerbotGameState* state,
                                    const uint8_t* deck, int deck_size);

int pokerbot_state_current_player(const PokerbotGameState* state);
int pokerbot_state_betting_round(const PokerbotGameState* state);
int pokerbot_state_is_terminal(const PokerbotGameState* state);
int pokerbot_state_terminal_reason(const PokerbotGameState* state);
int pokerbot_state_winner(const PokerbotGameState* state);

int64_t pokerbot_state_pot(const PokerbotGameState* state);
int64_t pokerbot_state_to_call(const PokerbotGameState* state, int player);
int64_t pokerbot_state_total_contribution(const PokerbotGameState* state,
                                          int player);
int64_t pokerbot_state_round_contribution(const PokerbotGameState* state,
                                          int player);

int pokerbot_state_board_count(const PokerbotGameState* state);
void pokerbot_state_board_cards(const PokerbotGameState* state, uint8_t* out);
void pokerbot_state_hole_cards(const PokerbotGameState* state, int player,
                               uint8_t* out);

int pokerbot_state_legal_actions(const PokerbotGameState* state, int* out,
                                 int max_actions);
int pokerbot_state_apply_action(PokerbotGameState* state, int action);

void pokerbot_state_payoffs(const PokerbotGameState* state, int64_t* out);

}
