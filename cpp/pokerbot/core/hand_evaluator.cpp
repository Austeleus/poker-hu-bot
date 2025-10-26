#include "hand_evaluator.h"

#include <algorithm>
#include <stdexcept>

namespace pokerbot::core {
namespace {

constexpr int kCategoryShift = 32;
constexpr int kRankShiftStep = 4;

uint64_t EncodeValue(int category, const std::vector<int>& ranks) {
  uint64_t value = static_cast<uint64_t>(category) << kCategoryShift;
  for (size_t i = 0; i < 5; ++i) {
    int rank = (i < ranks.size()) ? ranks[i] : -1;
    if (rank < 0) {
      rank = 0;
    }
    value |= static_cast<uint64_t>(rank & 0xF) << (kRankShiftStep * (4 - i));
  }
  return value;
}

int HighestStraightRank(uint16_t rank_mask) {
  for (int high = 12; high >= 4; --high) {
    bool straight = true;
    for (int offset = 0; offset < 5; ++offset) {
      int rank = high - offset;
      if ((rank_mask & (1 << rank)) == 0) {
        straight = false;
        break;
      }
    }
    if (straight) {
      return high;
    }
  }
  // Wheel straight (A-2-3-4-5) => treat as rank 3 (representing the 5).
  constexpr uint16_t wheel_mask = (1 << 12) | 0x1F;  // A + 2..5
  if ((rank_mask & wheel_mask) == wheel_mask) {
    return 3;  // rank index for 5
  }
  return -1;
}

std::vector<int> SortedRanksForSuit(const std::array<uint8_t, 5>& cards,
                                    int suit) {
  std::vector<int> ranks;
  ranks.reserve(5);
  for (uint8_t card : cards) {
    if (Suit(card) == suit) {
      ranks.push_back(Rank(card));
    }
  }
  std::sort(ranks.begin(), ranks.end(), std::greater<>());
  return ranks;
}

std::vector<int> SortedRanks(const std::array<uint8_t, 5>& cards) {
  std::vector<int> ranks;
  ranks.reserve(5);
  for (uint8_t card : cards) {
    ranks.push_back(Rank(card));
  }
  std::sort(ranks.begin(), ranks.end(), std::greater<>());
  return ranks;
}

}  // namespace

uint64_t EvaluateFiveCardHand(const std::array<uint8_t, 5>& cards) {
  int rank_counts[kRanks] = {0};
  int suit_counts[kSuits] = {0};
  uint16_t rank_mask = 0;
  for (uint8_t card : cards) {
    const int r = Rank(card);
    const int s = Suit(card);
    ++rank_counts[r];
    ++suit_counts[s];
    rank_mask |= (1 << r);
  }

  bool is_flush = false;
  int flush_suit = -1;
  for (int s = 0; s < kSuits; ++s) {
    if (suit_counts[s] == 5) {
      is_flush = true;
      flush_suit = s;
      break;
    }
  }

  const int straight_high = HighestStraightRank(rank_mask);
  const bool is_straight = straight_high != -1;

  int four_of_kind = -1;
  int three_of_kind = -1;
  std::vector<int> pairs;
  std::vector<int> singles;
  pairs.reserve(2);
  singles.reserve(5);

  for (int rank = 12; rank >= 0; --rank) {
    const int count = rank_counts[rank];
    if (count == 4) {
      four_of_kind = rank;
    } else if (count == 3) {
      if (three_of_kind == -1) {
        three_of_kind = rank;
      } else {
        singles.push_back(rank);
      }
    } else if (count == 2) {
      pairs.push_back(rank);
    } else if (count == 1) {
      singles.push_back(rank);
    }
  }

  if (is_flush && is_straight) {
    return EncodeValue(8, {straight_high});
  }

  if (four_of_kind != -1) {
    int kicker = singles.empty() ? 0 : singles.front();
    return EncodeValue(7, {four_of_kind, kicker});
  }

  if (three_of_kind != -1 && !pairs.empty()) {
    return EncodeValue(6, {three_of_kind, pairs.front()});
  }

  if (is_flush) {
    return EncodeValue(5, SortedRanksForSuit(cards, flush_suit));
  }

  if (is_straight) {
    return EncodeValue(4, {straight_high});
  }

  if (three_of_kind != -1) {
    std::vector<int> kickers{three_of_kind};
    for (int rank : singles) {
      kickers.push_back(rank);
      if (kickers.size() == 3) {
        break;
      }
    }
    while (kickers.size() < 3) {
      kickers.push_back(0);
    }
    return EncodeValue(3, kickers);
  }

  if (pairs.size() >= 2) {
    std::vector<int> kickers{pairs[0], pairs[1]};
    kickers.push_back(singles.empty() ? 0 : singles.front());
    return EncodeValue(2, kickers);
  }

  if (pairs.size() == 1) {
    std::vector<int> kickers{pairs[0]};
    for (int rank : singles) {
      kickers.push_back(rank);
      if (kickers.size() == 4) {
        break;
      }
    }
    while (kickers.size() < 4) {
      kickers.push_back(0);
    }
    return EncodeValue(1, kickers);
  }

  return EncodeValue(0, SortedRanks(cards));
}

uint64_t EvaluateBestHand(const std::vector<uint8_t>& cards) {
  if (cards.size() < 5 || cards.size() > 7) {
    throw std::invalid_argument("EvaluateBestHand requires 5 to 7 cards");
  }

  uint64_t best = 0;
  bool first = true;
  std::array<uint8_t, 5> combo{};

  const size_t n = cards.size();
  for (size_t i = 0; i < n; ++i) {
    for (size_t j = i + 1; j < n; ++j) {
      for (size_t k = j + 1; k < n; ++k) {
        for (size_t m = k + 1; m < n; ++m) {
          for (size_t p = m + 1; p < n; ++p) {
            combo[0] = cards[i];
            combo[1] = cards[j];
            combo[2] = cards[k];
            combo[3] = cards[m];
            combo[4] = cards[p];
            const uint64_t value = EvaluateFiveCardHand(combo);
            if (first || value > best) {
              best = value;
              first = false;
            }
          }
        }
      }
    }
  }
  return best;
}

int CompareHands(const std::vector<uint8_t>& first,
                 const std::vector<uint8_t>& second) {
  const uint64_t v1 = EvaluateBestHand(first);
  const uint64_t v2 = EvaluateBestHand(second);
  if (v1 > v2) {
    return 1;
  }
  if (v2 > v1) {
    return -1;
  }
  return 0;
}

}  // namespace pokerbot::core
