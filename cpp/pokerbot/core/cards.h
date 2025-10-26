#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace pokerbot::core {

constexpr int kDeckSize = 52;
constexpr int kRanks = 13;
constexpr int kSuits = 4;

inline int Rank(uint8_t card) { return static_cast<int>(card % kRanks); }

inline int Suit(uint8_t card) { return static_cast<int>(card / kRanks); }

inline bool IsValidCard(uint8_t card) { return card < kDeckSize; }

inline std::string CardToString(uint8_t card) {
  static constexpr std::array<const char*, kRanks> kRankNames{
      "2", "3", "4", "5", "6", "7", "8",
      "9", "T", "J", "Q", "K", "A"};
  static constexpr std::array<const char*, kSuits> kSuitNames{"c", "d", "h", "s"};
  if (!IsValidCard(card)) {
    return "??";
  }
  return std::string{kRankNames[Rank(card)]} + kSuitNames[Suit(card)];
}

}  // namespace pokerbot::core
