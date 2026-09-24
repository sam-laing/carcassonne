"""Thin ctypes wrapper for the optional native C++ Carcassonne engine."""

from __future__ import annotations

import ctypes
from dataclasses import dataclass
from enum import IntEnum
from pathlib import Path
from typing import Optional


class Phase(IntEnum):
    TILE = 0
    MEEPLE = 1
    FINISHED = 2


class ActionKind(IntEnum):
    PLACE_TILE = 0
    PLACE_MEEPLE = 1
    PASS = 2


class Feature(IntEnum):
    CITY = 0
    ROAD = 1
    MONASTERY = 2


class _CAction(ctypes.Structure):
    _fields_ = [
        ("kind", ctypes.c_uint8),
        ("row", ctypes.c_int16),
        ("column", ctypes.c_int16),
        ("rotation", ctypes.c_uint8),
        ("feature", ctypes.c_uint8),
        ("feature_index", ctypes.c_uint8),
    ]


class _CPlacedTile(ctypes.Structure):
    _fields_ = [
        ("row", ctypes.c_int16),
        ("column", ctypes.c_int16),
        ("tile", ctypes.c_uint8),
        ("rotation", ctypes.c_uint8),
        ("meeple_player", ctypes.c_uint8),
        ("meeple_feature", ctypes.c_uint8),
        ("meeple_group", ctypes.c_uint8),
    ]


@dataclass(frozen=True, slots=True)
class NativeAction:
    kind: ActionKind
    row: int = 0
    column: int = 0
    rotation: int = 0
    feature: Feature = Feature.CITY
    feature_index: int = 0

    def _native(self) -> _CAction:
        return _CAction(self.kind, self.row, self.column, self.rotation,
                        self.feature, self.feature_index)


@dataclass(frozen=True, slots=True)
class NativePlacedTile:
    row: int
    column: int
    tile: int
    rotation: int
    meeple_player: Optional[int]
    meeple_feature: Feature
    meeple_group: int


def _load_library(path: Optional[str] = None) -> ctypes.CDLL:
    if path is None:
        root = Path(__file__).resolve().parents[2]
        candidates = (
            root / "native" / "build" / "libcarcassonne_native.so",
            root / "native" / "build" / "libcarcassonne_native.dylib",
            root / "native" / "build" / "carcassonne_native.dll",
        )
        library = next((candidate for candidate in candidates if candidate.exists()), None)
        if library is None:
            raise ImportError("Native engine is not built; run: cmake -S native -B native/build && cmake --build native/build -j")
    else:
        library = Path(path)
    return ctypes.CDLL(str(library))


class NativeGame:
    """Owns a native game state. Copies are explicit through :meth:`clone`."""

    def __init__(self, seed: int = 0, players: int = 2, *, _library=None, _handle=None):
        self._handle = None
        self._lib = _library or _load_library()
        self._configure_api()
        self._handle = _handle or self._lib.carcassonne_game_new(seed, players)
        if not self._handle:
            raise ValueError("could not create native game (players must be 2..6)")

    def _configure_api(self) -> None:
        lib = self._lib
        lib.carcassonne_game_new.argtypes = [ctypes.c_uint64, ctypes.c_uint8]
        lib.carcassonne_game_new.restype = ctypes.c_void_p
        lib.carcassonne_game_clone.argtypes = [ctypes.c_void_p]
        lib.carcassonne_game_clone.restype = ctypes.c_void_p
        lib.carcassonne_game_free.argtypes = [ctypes.c_void_p]
        lib.carcassonne_game_reset.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
        lib.carcassonne_legal_actions.argtypes = [ctypes.c_void_p, ctypes.POINTER(_CAction), ctypes.c_size_t]
        lib.carcassonne_legal_actions.restype = ctypes.c_size_t
        lib.carcassonne_apply.argtypes = [ctypes.c_void_p, _CAction]
        lib.carcassonne_apply.restype = ctypes.c_int
        lib.carcassonne_play_random_game.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
        lib.carcassonne_play_random_game.restype = ctypes.c_uint32
        lib.carcassonne_board.argtypes = [ctypes.c_void_p, ctypes.POINTER(_CPlacedTile), ctypes.c_size_t]
        lib.carcassonne_board.restype = ctypes.c_size_t
        for name in ("phase", "players", "current_player", "current_tile", "meeples"):
            function = getattr(lib, f"carcassonne_{name}")
            function.argtypes = ([ctypes.c_void_p, ctypes.c_uint8]
                                 if name == "meeples" else [ctypes.c_void_p])
            function.restype = ctypes.c_uint8
        for name in ("placed_tiles", "remaining_tiles"):
            function = getattr(lib, f"carcassonne_{name}")
            function.argtypes = [ctypes.c_void_p]
            function.restype = ctypes.c_uint16
        lib.carcassonne_score.argtypes = [ctypes.c_void_p, ctypes.c_uint8]
        lib.carcassonne_score.restype = ctypes.c_int
        lib.carcassonne_hash.argtypes = [ctypes.c_void_p]
        lib.carcassonne_hash.restype = ctypes.c_uint64
        lib.carcassonne_tile_type_count.restype = ctypes.c_size_t
        lib.carcassonne_tile_name.argtypes = [ctypes.c_uint8]
        lib.carcassonne_tile_name.restype = ctypes.c_char_p

    def close(self) -> None:
        if self._handle:
            self._lib.carcassonne_game_free(self._handle)
            self._handle = None

    def __del__(self):
        self.close()

    def __enter__(self) -> "NativeGame":
        return self

    def __exit__(self, *_exc_info) -> None:
        self.close()

    def clone(self) -> "NativeGame":
        handle = self._lib.carcassonne_game_clone(self._handle)
        if not handle:
            raise MemoryError("could not clone native game")
        return NativeGame(_library=self._lib, _handle=handle)

    def reset(self, seed: int = 0) -> None:
        self._lib.carcassonne_game_reset(self._handle, seed)

    @property
    def phase(self) -> Phase:
        return Phase(self._lib.carcassonne_phase(self._handle))

    @property
    def current_player(self) -> int:
        return self._lib.carcassonne_current_player(self._handle)

    @property
    def players(self) -> int:
        return self._lib.carcassonne_players(self._handle)

    @property
    def current_tile(self) -> Optional[int]:
        tile = self._lib.carcassonne_current_tile(self._handle)
        return None if tile == 255 else tile

    @property
    def current_tile_name(self) -> Optional[str]:
        return None if self.current_tile is None else self.tile_name(self.current_tile)

    @property
    def placed_tiles(self) -> int:
        return self._lib.carcassonne_placed_tiles(self._handle)

    @property
    def remaining_tiles(self) -> int:
        return self._lib.carcassonne_remaining_tiles(self._handle)

    @property
    def state_hash(self) -> int:
        return self._lib.carcassonne_hash(self._handle)

    def tile_name(self, tile: int) -> str:
        name = self._lib.carcassonne_tile_name(tile)
        if name is None:
            raise IndexError(tile)
        return name.decode("utf-8")

    def board(self) -> list[NativePlacedTile]:
        count = self._lib.carcassonne_board(self._handle, None, 0)
        buffer = (_CPlacedTile * count)()
        self._lib.carcassonne_board(self._handle, buffer, count)
        return [NativePlacedTile(item.row, item.column, item.tile, item.rotation,
                                 None if item.meeple_player == 255 else item.meeple_player,
                                 Feature(item.meeple_feature), item.meeple_group)
                for item in buffer]

    def score(self, player: int) -> int:
        return self._lib.carcassonne_score(self._handle, player)

    def meeples(self, player: int) -> int:
        return self._lib.carcassonne_meeples(self._handle, player)

    def legal_actions(self) -> list[NativeAction]:
        count = self._lib.carcassonne_legal_actions(self._handle, None, 0)
        buffer = (_CAction * count)()
        self._lib.carcassonne_legal_actions(self._handle, buffer, count)
        return [NativeAction(ActionKind(item.kind), item.row, item.column,
                             item.rotation, Feature(item.feature), item.feature_index)
                for item in buffer]

    def step(self, action: NativeAction) -> None:
        if not self._lib.carcassonne_apply(self._handle, action._native()):
            raise ValueError(f"illegal action: {action}")

    def play_random_game(self, policy_seed: int = 0) -> int:
        return self._lib.carcassonne_play_random_game(self._handle, policy_seed)
