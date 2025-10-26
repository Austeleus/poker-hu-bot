#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "cards.h"

namespace pokerbot::core {

// Encodes a 5-card hand strength. Higher values are better.
uint64_t EvaluateFiveCardHand(const std::array<uint8_t, 5>& cards);

// Evaluates the strongest 5-card hand contained in the provided cards.
// Expects between 5 and 7 cards.
uint64_t EvaluateBestHand(const std::vector<uint8_t>& cards);

// Convenience helper for comparing two hands. Returns
//   1 if first hand is stronger,
//   0 if they tie,
//  -1 if second hand is stronger.
int CompareHands(const std::vector<uint8_t>& first,
                 const std::vector<uint8_t>& second);

}  // namespace pokerbot::core
