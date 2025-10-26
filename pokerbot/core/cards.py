"""Utilities for working with compact card representations."""

from __future__ import annotations

from typing import Iterable, List

RANKS = ["2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K", "A"]
SUITS = ["c", "d", "h", "s"]


def rank(card: int) -> int:
  return card % 13


def suit(card: int) -> int:
  return card // 13


def to_string(card: int) -> str:
  if not (0 <= card < 52):
    return "??"
  return f"{RANKS[rank(card)]}{SUITS[suit(card)]}"


def format_hand(cards: Iterable[int]) -> str:
  return " ".join(to_string(card) for card in cards)


def parse_card(token: str) -> int:
  token = token.strip().lower()
  if len(token) != 2:
    raise ValueError(f"Invalid card token: {token}")
  rank_char, suit_char = token[0].upper(), token[1]
  if rank_char not in RANKS and rank_char not in [r.upper() for r in RANKS]:
    raise ValueError(f"Unknown rank '{rank_char}'")
  if suit_char not in SUITS:
    raise ValueError(f"Unknown suit '{suit_char}'")
  rank_idx = RANKS.index(rank_char.upper())
  suit_idx = SUITS.index(suit_char)
  return suit_idx * 13 + rank_idx
