"""ctypes bindings for the native C++ limit holdem engine."""

from __future__ import annotations

import ctypes
import os
import sys
from pathlib import Path
from typing import Iterable, List, Optional, Sequence

__all__ = ["load_library", "NativeGameStateHolder", "ActionType"]


def _library_name() -> str:
  if sys.platform.startswith("linux"):
    return "libpokerbot_core.so"
  if sys.platform == "darwin":
    return "libpokerbot_core.dylib"
  if sys.platform.startswith("win"):
    return "pokerbot_core.dll"
  raise RuntimeError(f"Unsupported platform: {sys.platform}")


def _candidate_library_paths(lib_name: str) -> List[Path]:
  candidates: List[Path] = []
  env_path = os.environ.get("POKERBOT_CORE_LIB")
  if env_path:
    candidates.append(Path(env_path))

  this_file = Path(__file__).resolve()
  package_root = this_file.parents[1]
  repo_root = package_root.parent

  candidates.extend([
      package_root / "lib" / lib_name,
      repo_root / "lib" / lib_name,
      repo_root / "build" / lib_name,
      repo_root / "build" / "lib" / lib_name,
      this_file.parent / lib_name,
  ])
  seen = set()
  unique_candidates = []
  for path in candidates:
    if path in seen:
      continue
    seen.add(path)
    unique_candidates.append(path)
  return unique_candidates


_LIB: Optional[ctypes.CDLL] = None


def load_library() -> ctypes.CDLL:
  """Loads the native shared library, caching the handle."""
  global _LIB
  if _LIB is not None:
    return _LIB

  lib_name = _library_name()
  last_error: Optional[Exception] = None
  for candidate in _candidate_library_paths(lib_name):
    try:
      _LIB = ctypes.CDLL(str(candidate))
      break
    except OSError as exc:
      last_error = exc
  if _LIB is None:
    raise RuntimeError(
        f"Failed to load native core library '{lib_name}'. "
        "Build the project with CMake and set POKERBOT_CORE_LIB if needed."
    ) from last_error

  _configure_signatures(_LIB)
  return _LIB


def _configure_signatures(lib: ctypes.CDLL) -> None:
  lib.pokerbot_state_create.restype = ctypes.c_void_p
  lib.pokerbot_state_create.argtypes = []

  lib.pokerbot_state_destroy.restype = None
  lib.pokerbot_state_destroy.argtypes = [ctypes.c_void_p]

  lib.pokerbot_state_reset.restype = None
  lib.pokerbot_state_reset.argtypes = [ctypes.c_void_p, ctypes.c_uint64]

  lib.pokerbot_state_reset_with_deck.restype = None
  lib.pokerbot_state_reset_with_deck.argtypes = [
      ctypes.c_void_p,
      ctypes.POINTER(ctypes.c_uint8),
      ctypes.c_int,
  ]

  lib.pokerbot_state_current_player.restype = ctypes.c_int
  lib.pokerbot_state_current_player.argtypes = [ctypes.c_void_p]

  lib.pokerbot_state_betting_round.restype = ctypes.c_int
  lib.pokerbot_state_betting_round.argtypes = [ctypes.c_void_p]

  lib.pokerbot_state_is_terminal.restype = ctypes.c_int
  lib.pokerbot_state_is_terminal.argtypes = [ctypes.c_void_p]

  lib.pokerbot_state_terminal_reason.restype = ctypes.c_int
  lib.pokerbot_state_terminal_reason.argtypes = [ctypes.c_void_p]

  lib.pokerbot_state_winner.restype = ctypes.c_int
  lib.pokerbot_state_winner.argtypes = [ctypes.c_void_p]

  lib.pokerbot_state_pot.restype = ctypes.c_int64
  lib.pokerbot_state_pot.argtypes = [ctypes.c_void_p]

  lib.pokerbot_state_to_call.restype = ctypes.c_int64
  lib.pokerbot_state_to_call.argtypes = [ctypes.c_void_p, ctypes.c_int]

  lib.pokerbot_state_total_contribution.restype = ctypes.c_int64
  lib.pokerbot_state_total_contribution.argtypes = [ctypes.c_void_p, ctypes.c_int]

  lib.pokerbot_state_round_contribution.restype = ctypes.c_int64
  lib.pokerbot_state_round_contribution.argtypes = [ctypes.c_void_p, ctypes.c_int]

  lib.pokerbot_state_board_count.restype = ctypes.c_int
  lib.pokerbot_state_board_count.argtypes = [ctypes.c_void_p]

  lib.pokerbot_state_board_cards.restype = None
  lib.pokerbot_state_board_cards.argtypes = [
      ctypes.c_void_p,
      ctypes.POINTER(ctypes.c_uint8),
  ]

  lib.pokerbot_state_hole_cards.restype = None
  lib.pokerbot_state_hole_cards.argtypes = [
      ctypes.c_void_p,
      ctypes.c_int,
      ctypes.POINTER(ctypes.c_uint8),
  ]

  lib.pokerbot_state_legal_actions.restype = ctypes.c_int
  lib.pokerbot_state_legal_actions.argtypes = [
      ctypes.c_void_p,
      ctypes.POINTER(ctypes.c_int),
      ctypes.c_int,
  ]

  lib.pokerbot_state_apply_action.restype = ctypes.c_int
  lib.pokerbot_state_apply_action.argtypes = [ctypes.c_void_p, ctypes.c_int]

  lib.pokerbot_state_payoffs.restype = None
  lib.pokerbot_state_payoffs.argtypes = [
      ctypes.c_void_p,
      ctypes.POINTER(ctypes.c_int64),
  ]


class NativeGameStateHolder:
  """Thin RAII wrapper around the native game state pointer."""

  def __init__(self) -> None:
    self._lib = load_library()
    ptr = self._lib.pokerbot_state_create()
    if not ptr:
      raise RuntimeError("Failed to allocate native game state")
    self._ptr = ctypes.c_void_p(ptr)

  def close(self) -> None:
    if getattr(self, "_ptr", None):
      self._lib.pokerbot_state_destroy(self._ptr)
      self._ptr = None  # type: ignore[attr-defined]

  def __del__(self) -> None:
    try:
      self.close()
    except Exception:
      pass

  @property
  def ptr(self) -> ctypes.c_void_p:
    return self._ptr  # type: ignore[attr-defined]

  @property
  def lib(self) -> ctypes.CDLL:
    return self._lib

  # Convenience forwarding helpers -------------------------------------------------
  def reset(self, seed: int) -> None:
    self._lib.pokerbot_state_reset(self.ptr, ctypes.c_uint64(seed))

  def reset_with_deck(self, deck: Sequence[int]) -> None:
    if len(deck) < 52:
      raise ValueError("Deck must contain at least 52 cards")
    arr_type = ctypes.c_uint8 * len(deck)
    arr = arr_type(*deck)
    self._lib.pokerbot_state_reset_with_deck(self.ptr, arr, len(deck))

  def legal_actions(self, max_actions: int = 4) -> List[int]:
    buffer_type = ctypes.c_int * max_actions
    buffer = buffer_type()
    count = self._lib.pokerbot_state_legal_actions(self.ptr, buffer, max_actions)
    return [buffer[i] for i in range(count)]

  def apply_action(self, action: int) -> bool:
    return bool(self._lib.pokerbot_state_apply_action(self.ptr, action))

  def board_cards(self) -> List[int]:
    count = self._lib.pokerbot_state_board_count(self.ptr)
    if count <= 0:
      return []
    buffer_type = ctypes.c_uint8 * count
    buffer = buffer_type()
    self._lib.pokerbot_state_board_cards(self.ptr, buffer)
    return [buffer[i] for i in range(count)]

  def hole_cards(self, player: int) -> List[int]:
    buffer_type = ctypes.c_uint8 * 2
    buffer = buffer_type()
    self._lib.pokerbot_state_hole_cards(self.ptr, player, buffer)
    return [buffer[0], buffer[1]]

  def payoffs(self) -> List[int]:
    buffer_type = ctypes.c_int64 * 2
    buffer = buffer_type()
    self._lib.pokerbot_state_payoffs(self.ptr, buffer)
    return [int(buffer[0]), int(buffer[1])]

