# Native Carcassonne engine

This directory contains a headless C++17 base-game engine designed for simulation.
It uses a compact fixed board, a maintained placement frontier, precomputed tile
rotations, in-place transitions, deterministic seeded decks, and explicit cloning.

Implemented rules: all 72 base-deck tiles, legal edge matching, connected cities and
roads, normal meeple placement, completed and end-game scoring, monasteries, ties,
tile discards, and two to six players. Farmers, abbots, River, and Inns & Cathedrals
are intentionally not enabled yet.

For compatibility with the existing Python engine, all 72 tiles are shuffled and the
first drawn tile is placed at `(0, 0)`. An official fixed starting-tile mode can be
added independently of the simulation core.

Build and test:

```bash
cmake -S native -B native/build -DCMAKE_BUILD_TYPE=Release
cmake --build native/build -j
ctest --test-dir native/build --output-on-failure
native/build/carcassonne_benchmark 10000
```

Python usage (the wrapper uses only the standard library):

```python
from wingedsheep.carcassonne.native_game import NativeGame

game = NativeGame(seed=42, players=2)
while game.phase.name != "FINISHED":
    game.step(game.legal_actions()[0])
print(game.score(0), game.score(1))
```

For high-throughput random simulation, `game.play_random_game(policy_seed)` keeps the
entire loop in C++ and avoids per-action Python/FFI overhead.
