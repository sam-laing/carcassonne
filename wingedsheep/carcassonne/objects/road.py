from wingedsheep.carcassonne.objects.coordinate_with_side import CoordinateWithSide


class Road:
    def __init__(self, road_positions: list[CoordinateWithSide], finished: bool):
        self.road_positions = road_positions
        self.finished = finished
    def __repr__(self):
        return f"Road(road_positions={self.road_positions}, finished={self.finished})"



if __name__ == "__main__":
    x = Road([CoordinateWithSide(1, 2)], True)
    print(x)