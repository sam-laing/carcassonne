"""Play Carcassonne in a GUI, with the native C++ engine as the rules engine.

- The C++ engine (via NativeGame) decides legal moves, applies them, and scores.
- Tk + the base-game tile artwork render the board.
- You are player 0 and play by clicking; other seats play random moves.

Base game only (that is what the native engine implements).

Usage:
    python examples/play_gui.py [players] [seed]

Requires: the native lib built at native/build/libcarcassonne_native.{dylib,so},
and Pillow installed in the environment.
"""
import os
import random
import sys
import tkinter as tk

from PIL import Image, ImageTk

# Run from source so the native lib is found relative to the source tree.
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import wingedsheep
from wingedsheep.carcassonne.native_game import ActionKind, NativeAction, NativeGame, Phase
from wingedsheep.carcassonne.tile_sets.base_deck import base_tiles

HUMAN = 0
TILE = 64
IMAGES = os.path.join(wingedsheep.__path__[0], "carcassonne", "resources", "images")
PLAYER_COLORS = ["#2a6fdb", "#d6342c", "#222222", "#e5b700", "#1f9d55", "#d957a8"]
PLAYER_NAMES = ["Blue (you)", "Red", "Black", "Yellow", "Green", "Pink"]


class GuiGame:
    def __init__(self, players=2, seed=None):
        self.players = players
        self.seed = seed if seed is not None else random.randrange(100000)
        self.rng = random.Random(self.seed)
        self.game = NativeGame(seed=self.seed, players=players)

        self.tile_cache = {}            # (image, turns) -> PhotoImage
        self.legal_by_cell = {}         # (row, col) -> sorted list of legal rotations
        self.sel_cell = None
        self.preview_rot = 0
        self.r0 = self.c0 = 0           # top-left of the drawn region (board coords)

        self._build_ui()
        self.advance()

    # ---------- UI construction ----------
    def _build_ui(self):
        self.root = tk.Tk()
        self.root.title(f"Carcassonne (native engine) - seed {self.seed}")

        wrap = tk.Frame(self.root)
        wrap.pack(fill="both", expand=True)

        cframe = tk.Frame(wrap)
        cframe.pack(side="left", fill="both", expand=True)
        self.canvas = tk.Canvas(cframe, width=900, height=760, bg="#e9e4d0",
                                highlightthickness=0)
        hbar = tk.Scrollbar(cframe, orient="horizontal", command=self.canvas.xview)
        vbar = tk.Scrollbar(cframe, orient="vertical", command=self.canvas.yview)
        self.canvas.configure(xscrollcommand=hbar.set, yscrollcommand=vbar.set)
        vbar.pack(side="right", fill="y")
        hbar.pack(side="bottom", fill="x")
        self.canvas.pack(side="left", fill="both", expand=True)
        self.canvas.bind("<Button-1>", self.on_click)
        self.canvas.bind("<Double-Button-1>", self.on_double_click)

        panel = tk.Frame(wrap, width=260, padx=12, pady=12)
        panel.pack(side="right", fill="y")
        panel.pack_propagate(False)

        self.status = tk.Label(panel, text="", justify="left", anchor="w",
                               font=("Helvetica", 13), wraplength=236)
        self.status.pack(fill="x", pady=(0, 8))

        self.preview_label = tk.Label(panel, text="Current tile", font=("Helvetica", 11))
        self.preview_label.pack()
        self.preview_canvas = tk.Canvas(panel, width=TILE, height=TILE,
                                        bg="#e9e4d0", highlightthickness=1)
        self.preview_canvas.pack(pady=(2, 10))

        self.hint = tk.Label(panel, text="", justify="left", anchor="w",
                             fg="#444", wraplength=236)
        self.hint.pack(fill="x", pady=(0, 8))

        self.buttons = tk.Frame(panel)
        self.buttons.pack(fill="x")

        self.scores = tk.Label(panel, text="", justify="left", anchor="w",
                               font=("Helvetica", 12))
        self.scores.pack(side="bottom", fill="x", pady=8)

    # ---------- image helpers ----------
    def tile_photo(self, image_path, turns, size=TILE):
        key = (image_path, turns, size)
        if key not in self.tile_cache:
            img = Image.open(os.path.join(IMAGES, image_path)).convert("RGBA")
            img = img.resize((size, size)).rotate(-90 * turns)
            self.tile_cache[key] = ImageTk.PhotoImage(img)
        return self.tile_cache[key]

    # ---------- rendering ----------
    def draw(self):
        self.canvas.delete("all")
        placed = self.game.board()

        rows = [t.row for t in placed] + [c[0] for c in self.legal_by_cell]
        cols = [t.column for t in placed] + [c[1] for c in self.legal_by_cell]
        if not rows:
            rows, cols = [0], [0]
        self.r0, self.c0 = min(rows) - 1, min(cols) - 1
        r1, c1 = max(rows) + 1, max(cols) + 1
        w = (c1 - self.c0 + 1) * TILE
        h = (r1 - self.r0 + 1) * TILE
        self.canvas.configure(scrollregion=(0, 0, w, h))

        for t in placed:
            name = self.game.tile_name(t.tile)
            x = (t.column - self.c0) * TILE
            y = (t.row - self.r0) * TILE
            self.canvas.create_image(x, y, anchor="nw",
                                     image=self.tile_photo(base_tiles[name].image, t.rotation))
            if t.meeple_player is not None:
                self.canvas.create_oval(x + TILE/2 - 9, y + TILE/2 - 9,
                                        x + TILE/2 + 9, y + TILE/2 + 9,
                                        fill=PLAYER_COLORS[t.meeple_player], outline="white", width=2)

        # highlight legal placement cells (tile phase)
        for (r, c) in self.legal_by_cell:
            x = (c - self.c0) * TILE
            y = (r - self.r0) * TILE
            self.canvas.create_rectangle(x + 2, y + 2, x + TILE - 2, y + TILE - 2,
                                         outline="#1f9d55", width=2, dash=(4, 3))

        # preview the tile at the selected cell
        if self.sel_cell is not None:
            r, c = self.sel_cell
            x = (c - self.c0) * TILE
            y = (r - self.r0) * TILE
            name = self.game.current_tile_name
            self.canvas.create_image(x, y, anchor="nw",
                                     image=self.tile_photo(base_tiles[name].image, self.preview_rot))
            self.canvas.create_rectangle(x + 1, y + 1, x + TILE - 1, y + TILE - 1,
                                         outline="#d6342c", width=3)

    def draw_preview_panel(self):
        self.preview_canvas.delete("all")
        name = self.game.current_tile_name
        if name is not None:
            rot = self.preview_rot if self.sel_cell is not None else 0
            self.preview_canvas.create_image(0, 0, anchor="nw",
                                             image=self.tile_photo(base_tiles[name].image, rot))

    def refresh_text(self):
        p = self.game.current_player
        who = PLAYER_NAMES[p] if self.game.phase != Phase.FINISHED else "-"
        self.status.config(text=f"Turn: {who}\nPhase: {self.game.phase.name}\n"
                                f"Tiles left: {self.game.remaining_tiles}")
        self.scores.config(text="Scores\n" + "\n".join(
            f"  {PLAYER_NAMES[i]}: {self.game.score(i)}" for i in range(self.players)))
        self.draw_preview_panel()

    def clear_buttons(self):
        for w in self.buttons.winfo_children():
            w.destroy()

    # ---------- turn flow ----------
    def advance(self):
        self.refresh_text()
        if self.game.phase == Phase.FINISHED:
            self.finish()
            return
        if self.game.current_player == HUMAN:
            self.human_turn()
        else:
            self.clear_buttons()
            self.hint.config(text=f"{PLAYER_NAMES[self.game.current_player]} is thinking...")
            self.root.after(500, self.ai_move)

    def ai_move(self):
        self.game.step(self.rng.choice(self.game.legal_actions()))
        self.draw()
        self.advance()

    def human_turn(self):
        self.clear_buttons()
        actions = self.game.legal_actions()
        if self.game.phase == Phase.TILE:
            self.legal_by_cell = {}
            for a in actions:
                self.legal_by_cell.setdefault((a.row, a.column), []).append(a.rotation)
            for k in self.legal_by_cell:
                self.legal_by_cell[k] = sorted(set(self.legal_by_cell[k]))
            self.sel_cell = None
            self.hint.config(text="Click a dashed square to place the tile.\n"
                                  "Use Rotate to turn it, then Place (or double-click).")
            tk.Button(self.buttons, text="Rotate ⟳", command=self.rotate).pack(fill="x", pady=2)
            tk.Button(self.buttons, text="Place", command=self.place_tile).pack(fill="x", pady=2)
        else:  # MEEPLE phase
            self.legal_by_cell = {}
            self.sel_cell = None
            self.hint.config(text="Place a meeple, or skip.")
            for a in actions:
                if a.kind == ActionKind.PASS:
                    label = "Skip (no meeple)"
                else:
                    label = f"Meeple on {a.feature.name.lower()}"
                tk.Button(self.buttons, text=label,
                          command=lambda act=a: self.apply(act)).pack(fill="x", pady=2)
        self.draw()

    def rotate(self):
        if self.sel_cell is None:
            return
        rots = self.legal_by_cell[self.sel_cell]
        i = rots.index(self.preview_rot) if self.preview_rot in rots else -1
        self.preview_rot = rots[(i + 1) % len(rots)]
        self.draw()
        self.draw_preview_panel()

    def place_tile(self):
        if self.sel_cell is None:
            self.hint.config(text="Pick a dashed square first.")
            return
        r, c = self.sel_cell
        self.apply(NativeAction(ActionKind.PLACE_TILE, row=r, column=c, rotation=self.preview_rot))

    def apply(self, action):
        self.game.step(action)
        self.sel_cell = None
        self.legal_by_cell = {}
        self.draw()
        self.advance()

    # ---------- canvas events ----------
    def _cell_at(self, event):
        x = self.canvas.canvasx(event.x)
        y = self.canvas.canvasy(event.y)
        return (int(y // TILE) + self.r0, int(x // TILE) + self.c0)

    def on_click(self, event):
        if self.game.phase != Phase.TILE or self.game.current_player != HUMAN:
            return
        cell = self._cell_at(event)
        if cell in self.legal_by_cell:
            self.sel_cell = cell
            self.preview_rot = self.legal_by_cell[cell][0]
            self.draw()
            self.draw_preview_panel()

    def on_double_click(self, event):
        if self.game.phase == Phase.TILE and self.game.current_player == HUMAN:
            if self._cell_at(event) == self.sel_cell:
                self.place_tile()

    def finish(self):
        self.clear_buttons()
        self.sel_cell = None
        self.legal_by_cell = {}
        self.draw()
        scores = [(i, self.game.score(i)) for i in range(self.players)]
        best = max(s for _, s in scores)
        winners = [PLAYER_NAMES[i] for i, s in scores if s == best]
        result = "Winner: " + (winners[0] if len(winners) == 1 else "tie (" + ", ".join(winners) + ")")
        self.hint.config(text="Game over.\n" + result, font=("Helvetica", 13, "bold"))

    def run(self):
        self.root.mainloop()


def main():
    players = int(sys.argv[1]) if len(sys.argv) > 1 else 2
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else None
    GuiGame(players=players, seed=seed).run()


if __name__ == "__main__":
    main()
