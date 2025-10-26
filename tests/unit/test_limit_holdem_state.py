import os
import sys
import unittest
from pathlib import Path

from pokerbot.core.limit_holdem import ActionType, LimitHoldemState, TerminalReason


def _locate_library() -> bool:
  lib_name = {
      "linux": "libpokerbot_core.so",
      "darwin": "libpokerbot_core.dylib",
      "win32": "pokerbot_core.dll",
  }.get(sys.platform, "libpokerbot_core.so")
  repo_root = Path(__file__).resolve().parents[2]
  candidates = [
      repo_root / "build" / "lib" / lib_name,
      repo_root / "build" / lib_name,
      repo_root / "lib" / lib_name,
  ]
  return any(path.exists() for path in candidates)


@unittest.skipUnless(_locate_library(), "Native library not built")
class LimitHoldemStateTest(unittest.TestCase):
  def test_preflop_initial_state(self):
    state = LimitHoldemState(seed=1234)
    self.assertEqual(state.betting_round, 0)
    self.assertEqual(state.current_player, 0)
    self.assertEqual(state.pot, 3)
    actions = state.legal_actions()
    self.assertEqual(actions[0], ActionType.FOLD)
    self.assertIn(ActionType.CALL, actions)
    self.assertIn(ActionType.RAISE, actions)

  def test_preflop_call_advances_to_flop(self):
    state = LimitHoldemState(seed=42)
    self.assertTrue(state.apply_action(ActionType.CALL))
    self.assertEqual(state.betting_round, 1)
    self.assertEqual(len(state.board_cards()), 3)
    self.assertEqual(state.current_player, 1)
    self.assertEqual(state.legal_actions(),
                     [ActionType.CHECK, ActionType.BET])

  def test_showdown_deterministic_deck(self):
    state = LimitHoldemState(seed=1)
    hero = [12, 25]     # Ac, Ad
    villain = [11, 24]  # Kc, Kd
    board = [0, 16, 33, 48, 19]
    chosen = hero[:1] + villain[:1] + hero[1:] + villain[1:] + board
    remaining = [card for card in range(52) if card not in chosen]
    deck = chosen + remaining
    state.reset_with_deck(deck)

    self.assertTrue(state.apply_action(ActionType.CALL))
    self.assertEqual(state.legal_actions(), [ActionType.CHECK, ActionType.BET])

    for _ in range(3):
      self.assertTrue(state.apply_action(ActionType.CHECK))
      self.assertTrue(state.apply_action(ActionType.CHECK))

    self.assertTrue(state.is_terminal)
    self.assertEqual(state.terminal_reason, TerminalReason.SHOWDOWN)
    self.assertEqual(state.winner, 0)
    self.assertEqual(state.payoffs(), [2, -2])


if __name__ == "__main__":
  unittest.main()
