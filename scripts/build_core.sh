#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(
  cd "$(dirname "${BASH_SOURCE[0]}")/.."
  pwd
)"
BUILD_DIR="${ROOT_DIR}/build/lib"

echo "[pokerbot] Building native core library..."
mkdir -p "${BUILD_DIR}"

g++ -std=c++17 -O3 -fPIC \
  -I"${ROOT_DIR}/cpp" \
  "${ROOT_DIR}/cpp/pokerbot/core/c_api.cpp" \
  "${ROOT_DIR}/cpp/pokerbot/core/hand_evaluator.cpp" \
  "${ROOT_DIR}/cpp/pokerbot/core/limit_holdem_game.cpp" \
  -shared -o "${BUILD_DIR}/libpokerbot_core.so"

echo "[pokerbot] Output: ${BUILD_DIR}/libpokerbot_core.so"
