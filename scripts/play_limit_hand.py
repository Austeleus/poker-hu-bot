#!/usr/bin/env python3
"""Simple CLI to step through a single heads-up limit Hold'em hand."""

import argparse
import random
import sys
from pathlib import Path
from typing import List

if "pokerbot" not in sys.modules:
  # Ensure repo root is on sys.path when running as a script.
  repo_root = Path(__file__).resolve().parent.parent
  if str(repo_root) not in sys.path:
    sys.path.insert(0, str(repo_root))

from pokerbot.core.cards import format_hand, to_string
from pokerbot.core.limit_holdem import ActionType, LimitHoldemState, TerminalReason


def _action_prompt(actions: List[ActionType]) -> ActionType:
  options = {str(idx): action for idx, action in enumerate(actions)}
  names = {action.name.lower(): action for action in actions}

  while True:
    prompt = " / ".join(f"[{idx}] {action.name}" for idx, action in enumerate(actions))
    choice = input(f"Choose action ({prompt}): ").strip().lower()
    if choice in options:
      return options[choice]
    if choice in names:
      return names[choice]
    print("Unrecognised action. Please try again.")


def _describe_state(state: LimitHoldemState) -> None:
  print("-" * 60)
  print(f"Round: {state.betting_round}  Current player: {state.current_player}")
  print(f"Pot: {state.pot}  To call: {state.to_call(state.current_player)}")
  print(f"Board: {format_hand(state.board_cards()) or '--'}")
  for player in (0, 1):
    print(
        f"P{player} hole: {format_hand(state.hole_cards(player))} "
        f"(total={state.total_contribution(player)}, "
        f"round={state.round_contribution(player)})"
    )
  print("-" * 60)


def play_hand(seed: int) -> None:
  print(f"Starting new hand (seed={seed})")
  state = LimitHoldemState(seed=seed)
  while not state.is_terminal:
    _describe_state(state)
    actions = state.legal_actions()
    chosen = _action_prompt(actions)
    if not state.apply_action(chosen):
      print("Engine rejected action; please retry.")
      continue

  _describe_state(state)
  if state.terminal_reason == TerminalReason.SHOWDOWN:
    print("Hand reached showdown.")
  elif state.terminal_reason == TerminalReason.FOLD:
    print(f"Player {state.winner} wins via fold.")
  else:
    print("Hand terminated.")
  print(f"Winner: {state.winner}")
  payoffs = state.payoffs()
  print(f"Payoffs: P0={payoffs[0]}  P1={payoffs[1]}")


def main(argv: List[str]) -> int:
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument(
      "--seed", type=int, default=None, help="Optional RNG seed for the deck."
  )
  args = parser.parse_args(argv)
  seed = args.seed if args.seed is not None else random.getrandbits(64)
  play_hand(seed)
  return 0


if __name__ == "__main__":
  raise SystemExit(main(sys.argv[1:]))
