#include "c_api.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>

using pokerbot::core::ActionType;
using pokerbot::core::GameState;
using pokerbot::core::kDeckSize;
using pokerbot::core::kNumPlayers;

struct PokerbotGameState {
  GameState impl;
};

extern "C" {

PokerbotGameState* pokerbot_state_create() {
  try {
    return new PokerbotGameState();
  } catch (...) {
    return nullptr;
  }
}

void pokerbot_state_destroy(PokerbotGameState* state) {
  delete state;
}

void pokerbot_state_reset(PokerbotGameState* state, uint64_t seed) {
  if (!state) {
    return;
  }
  state->impl.Reset(seed);
}

void pokerbot_state_reset_with_deck(PokerbotGameState* state,
                                    const uint8_t* deck,
                                    int deck_size) {
  if (!state || !deck || deck_size < kDeckSize) {
    return;
  }
  std::array<uint8_t, kDeckSize> local_deck{};
  std::memcpy(local_deck.data(), deck, kDeckSize);
  state->impl.ResetWithDeck(local_deck);
}

int pokerbot_state_current_player(const PokerbotGameState* state) {
  return state ? state->impl.current_player() : -1;
}

int pokerbot_state_betting_round(const PokerbotGameState* state) {
  return state ? state->impl.betting_round() : -1;
}

int pokerbot_state_is_terminal(const PokerbotGameState* state) {
  return state ? (state->impl.is_terminal() ? 1 : 0) : 1;
}

int pokerbot_state_terminal_reason(const PokerbotGameState* state) {
  if (!state) {
    return 0;
  }
  return static_cast<int>(state->impl.terminal_reason());
}

int pokerbot_state_winner(const PokerbotGameState* state) {
  return state ? state->impl.winner() : -1;
}

int64_t pokerbot_state_pot(const PokerbotGameState* state) {
  return state ? state->impl.pot() : 0;
}

int64_t pokerbot_state_to_call(const PokerbotGameState* state, int player) {
  if (!state) {
    return 0;
  }
  try {
    return state->impl.ToCall(player);
  } catch (...) {
    return 0;
  }
}

int64_t pokerbot_state_total_contribution(const PokerbotGameState* state,
                                          int player) {
  if (!state) {
    return 0;
  }
  try {
    return state->impl.total_contribution(player);
  } catch (...) {
    return 0;
  }
}

int64_t pokerbot_state_round_contribution(const PokerbotGameState* state,
                                          int player) {
  if (!state) {
    return 0;
  }
  try {
    return state->impl.round_contribution(player);
  } catch (...) {
    return 0;
  }
}

int pokerbot_state_board_count(const PokerbotGameState* state) {
  return state ? state->impl.board_card_count() : 0;
}

void pokerbot_state_board_cards(const PokerbotGameState* state, uint8_t* out) {
  if (!state || !out) {
    return;
  }
  const auto cards = state->impl.board_cards();
  std::copy(cards.begin(), cards.end(), out);
}

void pokerbot_state_hole_cards(const PokerbotGameState* state, int player,
                               uint8_t* out) {
  if (!state || !out) {
    return;
  }
  try {
    const auto& cards = state->impl.hole_cards(player);
    out[0] = cards[0];
    out[1] = cards[1];
  } catch (...) {
    out[0] = out[1] = 0;
  }
}

int pokerbot_state_legal_actions(const PokerbotGameState* state, int* out,
                                 int max_actions) {
  if (!state) {
    return 0;
  }
  const auto actions = state->impl.LegalActions();
  const int count =
      std::min<int>(static_cast<int>(actions.size()), max_actions);
  for (int i = 0; i < count; ++i) {
    out[i] = static_cast<int>(actions[i]);
  }
  return count;
}

int pokerbot_state_apply_action(PokerbotGameState* state, int action) {
  if (!state) {
    return 0;
  }
  const auto typed_action = static_cast<ActionType>(action);
  return state->impl.ApplyAction(typed_action) ? 1 : 0;
}

void pokerbot_state_payoffs(const PokerbotGameState* state, int64_t* out) {
  if (!state || !out) {
    return;
  }
  const auto payoffs = state->impl.payoffs();
  for (int i = 0; i < kNumPlayers; ++i) {
    out[i] = payoffs[i];
  }
}

}  // extern "C"
