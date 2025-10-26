"""High-level Python wrapper around the native limit Hold'em engine."""

from __future__ import annotations

import random
from enum import IntEnum
from typing import Iterable, List, Optional, Sequence

from .native import NativeGameStateHolder, load_library

__all__ = ["ActionType", "TerminalReason", "LimitHoldemState"]


class ActionType(IntEnum):
  FOLD = 0
  CHECK = 1
  CALL = 2
  BET = 3
  RAISE = 4


class TerminalReason(IntEnum):
  NONE = 0
  FOLD = 1
  SHOWDOWN = 2


class LimitHoldemState:
  """Encapsulates a single heads-up limit Hold'em hand."""

  def __init__(self, seed: Optional[int] = None) -> None:
    self._holder = NativeGameStateHolder()
    if seed is None:
      seed = random.getrandbits(64)
    self.reset(seed=seed)

  # --------------------------------------------------------------------------- #
  # Lifecycle
  # --------------------------------------------------------------------------- #
  def reset(self, seed: Optional[int] = None) -> None:
    if seed is None:
      seed = random.getrandbits(64)
    self._holder.reset(seed)

  def reset_with_deck(self, deck: Sequence[int]) -> None:
    """Deterministic reset with a predefined deck ordering."""
    self._holder.reset_with_deck(deck)

  def close(self) -> None:
    self._holder.close()

  # --------------------------------------------------------------------------- #
  # State queries
  # --------------------------------------------------------------------------- #
  @property
  def current_player(self) -> int:
    return self._holder.lib.pokerbot_state_current_player(self._holder.ptr)

  @property
  def betting_round(self) -> int:
    return self._holder.lib.pokerbot_state_betting_round(self._holder.ptr)

  @property
  def is_terminal(self) -> bool:
    return bool(self._holder.lib.pokerbot_state_is_terminal(self._holder.ptr))

  @property
  def terminal_reason(self) -> TerminalReason:
    value = self._holder.lib.pokerbot_state_terminal_reason(self._holder.ptr)
    return TerminalReason(value)

  @property
  def winner(self) -> int:
    return self._holder.lib.pokerbot_state_winner(self._holder.ptr)

  @property
  def pot(self) -> int:
    return int(self._holder.lib.pokerbot_state_pot(self._holder.ptr))

  def to_call(self, player: int) -> int:
    return int(
        self._holder.lib.pokerbot_state_to_call(self._holder.ptr, int(player))
    )

  def total_contribution(self, player: int) -> int:
    return int(
        self._holder.lib.pokerbot_state_total_contribution(
            self._holder.ptr, int(player)
        )
    )

  def round_contribution(self, player: int) -> int:
    return int(
        self._holder.lib.pokerbot_state_round_contribution(
            self._holder.ptr, int(player)
        )
    )

  def hole_cards(self, player: int) -> List[int]:
    return self._holder.hole_cards(int(player))

  def board_cards(self) -> List[int]:
    return self._holder.board_cards()

  def payoffs(self) -> List[int]:
    return self._holder.payoffs()

  # --------------------------------------------------------------------------- #
  # Actions
  # --------------------------------------------------------------------------- #
  def legal_actions(self) -> List[ActionType]:
    return [ActionType(value) for value in self._holder.legal_actions()]

  def apply_action(self, action: ActionType) -> bool:
    return self._holder.apply_action(int(action))

  # --------------------------------------------------------------------------- #
  # Convenience helpers
  # --------------------------------------------------------------------------- #
  def play_sequence(self, actions: Iterable[ActionType]) -> None:
    for action in actions:
      success = self.apply_action(action)
      if not success:
        raise ValueError(f"Illegal action {action} in current state")
      if self.is_terminal:
        break

  def __repr__(self) -> str:
    return (
        "LimitHoldemState("
        f"round={self.betting_round}, "
        f"current_player={self.current_player}, "
        f"pot={self.pot}, "
        f"terminal={self.is_terminal})"
    )

  def __del__(self) -> None:
    try:
      self.close()
    except Exception:
      pass
