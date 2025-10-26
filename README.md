# poker-hu-bot

Heads-up limit Texas Hold'em poker bot with an MCCFR core implemented in C++ and a Python orchestration layer.

## Getting Started

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install --upgrade pip
pip install -e .
```

### Building the native core

```bash
./scripts/build_core.sh
```

If you prefer CMake:

```bash
cmake -S . -B build
cmake --build build
```

The compiled shared library will be placed in `build/lib`. Set `POKERBOT_CORE_LIB` to point at the shared object if you move it elsewhere.

### Running tests

```bash
python3 -m unittest discover -s tests/unit
```

### Manual interaction

```bash
./scripts/build_core.sh  # if not already built
./scripts/play_limit_hand.py --seed 123
```

## Project Layout

- `pokerbot/core`: Python wrappers around the native C++ engine, high-level environment helpers.
- `cpp/pokerbot`: C++ implementation of the game mechanics and (later) core CFR algorithms.
- `pokerbot/training`, `pokerbot/evaluation`, `pokerbot/runtime`: Orchestration layers that will call into the native module.
- `docs/`: Design notes and roadmaps.
- `tests/`: Unit and integration tests.
