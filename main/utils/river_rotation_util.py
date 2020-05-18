from typing import Optional, Set

from main.carcassonne_game_state import CarcassonneGameState
from main.objects.rotation import Rotation
from main.objects.side import Side
from main.objects.tile import Tile
from main.utils.side_modification_util import SideModificationUtil


class RiverRotationUtil:

    @staticmethod
    def get_river_rotation(game_state: CarcassonneGameState, tile: Tile) -> Rotation:
        if tile.has_river() and game_state.last_tile_action is not None:
            river_rotation: Rotation = RiverRotationUtil.get_river_rotation_tile(
                previous_tile=game_state.last_tile_action.tile,
                new_tile=tile)
            if river_rotation != Rotation.NONE:
                return river_rotation
            else:
                return game_state.last_river_rotation

    @staticmethod
    def get_connecting_side(previous_river_sides: [Side], river_sides: [Side]) -> Optional[Side]:
        for side in river_sides:
            if SideModificationUtil.opposite_side(side) in previous_river_sides:
                return side
        return None

    @staticmethod
    def get_river_rotation_tile(previous_tile: Tile, new_tile: Tile):
        previous_river_ends: Set[Side] = set(previous_tile.get_river_ends())
        river_ends: Set[Side] = set(new_tile.get_river_ends())
        return RiverRotationUtil.get_river_rotation_ends(previous_river_ends=previous_river_ends, river_ends=river_ends)

    @staticmethod
    def get_river_rotation_ends(previous_river_ends: Set[Side], river_ends: Set[Side]):
        connecting_side: Side = RiverRotationUtil.get_connecting_side(previous_river_ends, river_ends)
        non_connecting_side: Side = river_ends.difference([connecting_side]).pop()

        if SideModificationUtil.turn_side(non_connecting_side, 1) == connecting_side:
            return Rotation.CLOCKWISE
        elif SideModificationUtil.turn_side(non_connecting_side, 3) == connecting_side:
            return Rotation.COUNTER_CLOCKWISE
        else:
            return Rotation.NONE
