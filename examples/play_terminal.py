"""Play Carcassonne in the terminal against the native C++ engine.

You (player 0) make moves by typing a number; other seats play random moves.
Base game only -- that's what the native engine implements.

Usage:
    python examples/play_terminal.py [players] [seed]

Needs the native library built at native/build/libcarcassonne_native.{dylib,so}.
"""
import os
import random
import sys

# Run from source: put the repo root on the path (the native lib is found
# relative to the source tree, so an installed copy wouldn't locate it).
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from wingedsheep.carcassonne.native_game import (
    ActionKind, NativeGame, Phase,
)

HUMAN = 0  # which seat you control


def render(game):
    tiles = game.board()
    if not tiles:
        print("  (empty board)")
        return
    rows = [t.row for t in tiles]
    cols = [t.column for t in tiles]
    r0, r1, c0, c1 = min(rows), max(rows), min(cols), max(cols)
    grid = {(t.row, t.column): t for t in tiles}

    header = "     " + "".join(f"{c:>4}" for c in range(c0, c1 + 1))
    print(header)
    for r in range(r0, r1 + 1):
        cells = []
        for c in range(c0, c1 + 1):
            t = grid.get((r, c))
            if t is None:
                cells.append("  . ")
            else:
                meeple = "*" if t.meeple_player is not None else " "
                cells.append(f"{t.tile:>3}{meeple}")
        print(f"{r:>4} " + "".join(cells))
    print()
    # legend of tiles currently on the board
    seen = {}
    for t in tiles:
        seen[t.tile] = game.tile_name(t.tile)
    print("  tiles on board: " + ", ".join(f"{i}={n}" for i, n in sorted(seen.items())))


def describe(action):
    if action.kind == ActionKind.PLACE_TILE:
        return f"place tile at (row {action.row}, col {action.column}), rotation {action.rotation}"
    if action.kind == ActionKind.PLACE_MEEPLE:
        return f"place meeple on {action.feature.name.lower()} #{action.feature_index}"
    if action.kind == ActionKind.PASS:
        return "pass (place no meeple)"
    return str(action)


def choose_human(game, actions):
    print(f"\n  Phase: {game.phase.name}", end="")
    if game.current_tile is not None:
        print(f"   |   current tile: {game.current_tile_name}", end="")
    print(f"   |   your meeples left: {game.meeples(HUMAN)}\n")
    for i, a in enumerate(actions):
        print(f"   [{i:>3}] {describe(a)}")
    while True:
        raw = input(f"\n  Your move 0-{len(actions) - 1} (q to quit): ").strip()
        if raw.lower() in ("q", "quit", "exit"):
            sys.exit(0)
        if raw.isdigit() and 0 <= int(raw) < len(actions):
            return actions[int(raw)]
        print("  invalid choice, try again.")


def main():
    players = int(sys.argv[1]) if len(sys.argv) > 1 else 2
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else random.randrange(100000)
    rng = random.Random(seed)

    game = NativeGame(seed=seed, players=players)
    print(f"Carcassonne (native engine) | {players} players | seed {seed}")
    print(f"You are player {HUMAN}. Other seats play random moves.\n")

    while game.phase != Phase.FINISHED:
        actions = game.legal_actions()
        player = game.current_player
        if player == HUMAN:
            print("=" * 72)
            print(f"Scores: " + "  ".join(f"P{p}={game.score(p)}" for p in range(players))
                  + f"   |   tiles left: {game.remaining_tiles}")
            render(game)
            action = choose_human(game, actions)
        else:
            action = rng.choice(actions)
            print(f"  Player {player}: {describe(action)}")
        game.step(action)

    print("\n" + "=" * 72)
    print("GAME OVER")
    render(game)
    scores = [(p, game.score(p)) for p in range(players)]
    for p, s in scores:
        tag = "  <- you" if p == HUMAN else ""
        print(f"  Player {p}: {s}{tag}")
    best = max(s for _, s in scores)
    winners = [p for p, s in scores if s == best]
    print("\n  Winner: " + (f"Player {winners[0]}" if len(winners) == 1
                            else "tie between " + ", ".join(f"P{p}" for p in winners)))


if __name__ == "__main__":
    main()
