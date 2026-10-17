#include "carcassonne/engine.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace carcassonne {
namespace {

using T = Terrain;
constexpr std::int8_t N = -1;

constexpr std::array<T, 4> E(T a, T b, T c, T d) { return {a, b, c, d}; }
constexpr std::array<std::int8_t, 4> G(std::int8_t a = N, std::int8_t b = N,
                                       std::int8_t c = N, std::int8_t d = N) {
    return {a, b, c, d};
}

constexpr std::array<TileDefinition, 32> kTiles{{
    {"chapel_with_road", 2, E(T::Grass,T::Grass,T::Road,T::Grass), G(), G(N,N,0,N), false, true},
    {"chapel", 4, E(T::Grass,T::Grass,T::Grass,T::Grass), G(), G(), false, true},
    {"full_city_with_shield", 1, E(T::City,T::City,T::City,T::City), G(0,0,0,0), G(), true, false},
    {"city_top_straight_road", 4, E(T::City,T::Road,T::Grass,T::Road), G(0), G(N,0,N,0), false, false},
    {"city_top", 4, E(T::City,T::Grass,T::Grass,T::Grass), G(0), G(), false, false},
    {"city_top_flowers", 1, E(T::City,T::Grass,T::Grass,T::Grass), G(0), G(), false, false},
    {"city_narrow_shield", 2, E(T::Grass,T::City,T::Grass,T::City), G(N,0,N,0), G(), true, false},
    {"city_narrow", 1, E(T::Grass,T::City,T::Grass,T::City), G(N,0,N,0), G(), false, false},
    {"city_left_right", 2, E(T::Grass,T::City,T::Grass,T::City), G(N,0,N,1), G(), false, false},
    {"city_top_bottom_flowers", 1, E(T::City,T::Grass,T::City,T::Grass), G(0,N,1,N), G(), false, false},
    {"city_top_right", 1, E(T::City,T::City,T::Grass,T::Grass), G(0,1), G(), false, false},
    {"city_top_left_flowers", 1, E(T::City,T::Grass,T::Grass,T::City), G(0,N,N,1), G(), false, false},
    {"city_top_road_bend_right", 3, E(T::City,T::Road,T::Road,T::Grass), G(0), G(N,0,0,N), false, false},
    {"city_top_road_bend_left", 3, E(T::City,T::Grass,T::Road,T::Road), G(0), G(N,N,0,0), false, false},
    {"city_top_crossroads", 3, E(T::City,T::Road,T::Road,T::Road), G(0), G(N,0,1,2), false, false},
    {"city_diagonal_top_right_shield", 1, E(T::City,T::City,T::Grass,T::Grass), G(0,0), G(), true, false},
    {"city_diagonal_top_right_shield_flowers", 1, E(T::City,T::City,T::Grass,T::Grass), G(0,0), G(), true, false},
    {"city_diagonal_top_right", 2, E(T::City,T::City,T::Grass,T::Grass), G(0,0), G(), false, false},
    {"city_diagonal_top_right_flowers", 1, E(T::City,T::City,T::Grass,T::Grass), G(0,0), G(), false, false},
    {"city_diagonal_top_left_shield_road", 2, E(T::City,T::Road,T::Road,T::City), G(0,N,N,0), G(N,0,0,N), true, false},
    {"city_diagonal_top_left_road", 3, E(T::City,T::Road,T::Road,T::City), G(0,N,N,0), G(N,0,0,N), false, false},
    {"city_bottom_grass_shield", 1, E(T::City,T::City,T::Grass,T::City), G(0,0,N,0), G(), true, false},
    {"city_bottom_grass", 2, E(T::City,T::City,T::Grass,T::City), G(0,0,N,0), G(), false, false},
    {"city_bottom_grass_flowers", 1, E(T::City,T::City,T::Grass,T::City), G(0,0,N,0), G(), false, false},
    {"city_bottom_road_shield", 2, E(T::City,T::City,T::Road,T::City), G(0,0,N,0), G(N,N,0,N), true, false},
    {"city_bottom_road", 1, E(T::City,T::City,T::Road,T::City), G(0,0,N,0), G(N,N,0,N), false, false},
    {"straight_road", 7, E(T::Road,T::Grass,T::Road,T::Grass), G(), G(0,N,0,N), false, false},
    {"straight_road_flowers", 1, E(T::Road,T::Grass,T::Road,T::Grass), G(), G(0,N,0,N), false, false},
    {"bent_road", 8, E(T::Grass,T::Grass,T::Road,T::Road), G(), G(N,N,0,0), false, false},
    {"bent_road_flowers", 1, E(T::Grass,T::Grass,T::Road,T::Road), G(), G(N,N,0,0), false, false},
    {"three_split_road", 4, E(T::Grass,T::Road,T::Road,T::Road), G(), G(N,0,1,2), false, false},
    {"crossroads", 1, E(T::Road,T::Road,T::Road,T::Road), G(), G(0,1,2,3), false, false},
}};

// Fields live on 8 half-edges per tile, clockwise from the top side's left half:
// 0-1 top, 2-3 right, 4-5 bottom, 6-7 left. `cities` is, per field group, a bitmask
// of the tile's city groups bordering that field. Same order as kTiles.
struct FieldDefinition {
    std::array<std::int8_t, 8> groups;
    std::array<std::uint8_t, 4> cities;
};

constexpr std::array<std::int8_t, 8> F(std::int8_t a, std::int8_t b, std::int8_t c, std::int8_t d,
                                       std::int8_t e, std::int8_t f, std::int8_t g, std::int8_t h) {
    return {a, b, c, d, e, f, g, h};
}

constexpr std::array<FieldDefinition, 32> kFields{{
    {F(0,0,0,0,0,0,0,0), {0,0,0,0}},  // chapel_with_road
    {F(0,0,0,0,0,0,0,0), {0,0,0,0}},  // chapel
    {F(N,N,N,N,N,N,N,N), {0,0,0,0}},  // full_city_with_shield
    {F(N,N,0,1,1,1,1,0), {1,0,0,0}},  // city_top_straight_road
    {F(N,N,0,0,0,0,0,0), {1,0,0,0}},  // city_top
    {F(N,N,0,0,0,0,0,0), {1,0,0,0}},  // city_top_flowers
    {F(0,0,N,N,1,1,N,N), {1,1,0,0}},  // city_narrow_shield
    {F(0,0,N,N,1,1,N,N), {1,1,0,0}},  // city_narrow
    {F(0,0,N,N,0,0,N,N), {3,0,0,0}},  // city_left_right
    {F(N,N,0,0,N,N,0,0), {3,0,0,0}},  // city_top_bottom_flowers
    {F(N,N,N,N,0,0,0,0), {3,0,0,0}},  // city_top_right
    {F(N,N,0,0,0,0,N,N), {3,0,0,0}},  // city_top_left_flowers
    {F(N,N,0,1,1,0,0,0), {1,0,0,0}},  // city_top_road_bend_right
    {F(N,N,0,0,0,1,1,0), {1,0,0,0}},  // city_top_road_bend_left
    {F(N,N,0,2,2,1,1,0), {1,0,0,0}},  // city_top_crossroads
    {F(N,N,N,N,0,0,0,0), {1,0,0,0}},  // city_diagonal_top_right_shield
    {F(N,N,N,N,0,0,0,0), {1,0,0,0}},  // city_diagonal_top_right_shield_flowers
    {F(N,N,N,N,0,0,0,0), {1,0,0,0}},  // city_diagonal_top_right
    {F(N,N,N,N,0,0,0,0), {1,0,0,0}},  // city_diagonal_top_right_flowers
    {F(N,N,0,1,1,0,N,N), {1,0,0,0}},  // city_diagonal_top_left_shield_road
    {F(N,N,0,1,1,0,N,N), {1,0,0,0}},  // city_diagonal_top_left_road
    {F(N,N,N,N,0,0,N,N), {1,0,0,0}},  // city_bottom_grass_shield
    {F(N,N,N,N,0,0,N,N), {1,0,0,0}},  // city_bottom_grass
    {F(N,N,N,N,0,0,N,N), {1,0,0,0}},  // city_bottom_grass_flowers
    {F(N,N,N,N,1,0,N,N), {1,1,0,0}},  // city_bottom_road_shield
    {F(N,N,N,N,1,0,N,N), {1,1,0,0}},  // city_bottom_road
    {F(0,1,1,1,1,0,0,0), {0,0,0,0}},  // straight_road
    {F(0,1,1,1,1,0,0,0), {0,0,0,0}},  // straight_road_flowers
    {F(1,1,1,1,1,0,0,1), {0,0,0,0}},  // bent_road
    {F(1,1,1,1,1,0,0,1), {0,0,0,0}},  // bent_road_flowers
    {F(2,2,2,1,1,0,0,2), {0,0,0,0}},  // three_split_road
    {F(2,3,3,1,1,0,0,2), {0,0,0,0}},  // crossroads
}};

}  // namespace

const std::array<TileDefinition, 32>& Game::tile_catalog() { return kTiles; }

const std::array<std::array<Game::RotatedTile, 4>, 32>& Game::rotated_catalog() {
    static const auto rotations = [] {
        std::array<std::array<RotatedTile, 4>, 32> result{};
        for (std::size_t tile = 0; tile < kTiles.size(); ++tile) {
            for (int rotation = 0; rotation < 4; ++rotation) {
                auto& out = result[tile][rotation];
                out.shield = kTiles[tile].shield;
                out.monastery = kTiles[tile].monastery;
                out.field_cities = kFields[tile].cities;
                for (int side = 0; side < 4; ++side) {
                    const int destination = (side + rotation) & 3;
                    out.edges[destination] = kTiles[tile].edges[side];
                    out.city_groups[destination] = kTiles[tile].city_groups[side];
                    out.road_groups[destination] = kTiles[tile].road_groups[side];
                }
                for (int half = 0; half < 8; ++half) {
                    out.field_groups[(half + 2 * rotation) & 7] = kFields[tile].groups[half];
                }
            }
        }
        return result;
    }();
    return rotations;
}

Game::Game(std::uint64_t seed, std::uint8_t players, bool farmers)
    : players_(players), farmers_(farmers) {
    if (players < 2 || players > kMaxPlayers) {
        throw std::invalid_argument("players must be between 2 and 6");
    }
    reset(seed);
}

void Game::reset(std::uint64_t seed) {
    board_.fill(Cell{});
    frontier_.fill(0);
    frontier_cells_.clear();
    scores_.fill(0);
    meeples_.fill(0);
    std::fill_n(meeples_.begin(), players_, std::uint8_t{7});
    current_player_ = 0;
    current_tile_ = kEmpty;
    phase_ = Phase::Tile;
    placed_tiles_ = 0;
    last_cell_ = -1;

    std::size_t position = 0;
    for (std::size_t tile = 0; tile < kTiles.size(); ++tile) {
        for (int count = 0; count < kTiles[tile].count; ++count) {
            deck_[position++] = static_cast<std::uint8_t>(tile);
        }
    }
    assert(position == deck_.size());
    std::mt19937_64 rng(seed);
    std::shuffle(deck_.begin(), deck_.end(), rng);
    deck_position_ = 0;
    draw_next_tile();
}

std::uint16_t Game::remaining_tiles() const noexcept {
    return static_cast<std::uint16_t>(deck_.size() - deck_position_);
}

int Game::neighbor(int cell, int side) noexcept {
    static constexpr std::array<int, 4> delta{-kBoardSize, 1, kBoardSize, -1};
    const int row = cell / kBoardSize;
    const int column = cell % kBoardSize;
    if ((side == 0 && row == 0) || (side == 1 && column == kBoardSize - 1) ||
        (side == 2 && row == kBoardSize - 1) || (side == 3 && column == 0)) {
        return -1;
    }
    return cell + delta[side];
}

const Game::RotatedTile& Game::tile_at(int cell) const noexcept {
    const Cell& placed = board_[cell];
    return rotated_catalog()[placed.tile][placed.rotation];
}

bool Game::tile_fits(int cell, std::uint8_t rotation) const noexcept {
    const auto& candidate = rotated_catalog()[current_tile_][rotation];
    bool adjacent = false;
    for (int side = 0; side < 4; ++side) {
        const int other = neighbor(cell, side);
        if (other < 0 || board_[other].tile == kEmpty) continue;
        adjacent = true;
        if (candidate.edges[side] != tile_at(other).edges[opposite(side)]) return false;
    }
    return adjacent;
}

void Game::legal_actions(std::vector<Action>& output) const {
    output.clear();
    if (phase_ == Phase::Finished) return;

    if (phase_ == Phase::Tile) {
        if (placed_tiles_ == 0) {
            output.push_back({ActionKind::PlaceTile, 0, 0, 0, Feature::City, 0});
            return;
        }
        for (int cell : frontier_cells_) {
            if (!frontier_[cell] || board_[cell].tile != kEmpty) continue;
            for (std::uint8_t rotation = 0; rotation < 4; ++rotation) {
                if (tile_fits(cell, rotation)) {
                    output.push_back({ActionKind::PlaceTile,
                                      static_cast<std::int16_t>(cell / kBoardSize - kCenter),
                                      static_cast<std::int16_t>(cell % kBoardSize - kCenter),
                                      rotation, Feature::City, 0});
                }
            }
        }
        if (output.empty()) output.push_back({ActionKind::Pass});
        return;
    }

    if (meeples_[current_player_] > 0) {
        const auto& tile = tile_at(last_cell_);
        for (Feature feature : {Feature::City, Feature::Road}) {
            const auto& groups = feature == Feature::City ? tile.city_groups : tile.road_groups;
            std::array<bool, 4> emitted{};
            for (int side = 0; side < 4; ++side) {
                const auto group = groups[side];
                if (group < 0 || emitted[group]) continue;
                emitted[group] = true;
                const auto result = trace_feature(last_cell_, feature, static_cast<std::uint8_t>(group));
                const int followers = std::accumulate(result.followers.begin(), result.followers.end(), 0);
                if (followers == 0) {
                    output.push_back({ActionKind::PlaceMeeple, 0, 0, 0, feature,
                                      static_cast<std::uint8_t>(group)});
                }
            }
        }
        if (tile.monastery) {
            output.push_back({ActionKind::PlaceMeeple, 0, 0, 0, Feature::Monastery, 0});
        }
        if (farmers_) {
            std::array<bool, 4> emitted{};
            for (int half = 0; half < 8; ++half) {
                const auto group = tile.field_groups[half];
                if (group < 0 || emitted[group]) continue;
                emitted[group] = true;
                const auto result = trace_field(last_cell_, static_cast<std::uint8_t>(group));
                const int followers = std::accumulate(result.followers.begin(), result.followers.end(), 0);
                if (followers == 0) {
                    output.push_back({ActionKind::PlaceMeeple, 0, 0, 0, Feature::Field,
                                      static_cast<std::uint8_t>(group)});
                }
            }
        }
    }
    output.push_back({ActionKind::Pass});
}

std::vector<Action> Game::legal_actions() const {
    std::vector<Action> actions;
    actions.reserve(128);
    legal_actions(actions);
    return actions;
}

bool Game::apply(const Action& action) {
    const auto actions = legal_actions();
    if (std::find(actions.begin(), actions.end(), action) == actions.end()) return false;
    apply_unchecked(action);
    return true;
}

void Game::add_frontier_neighbors(int cell) {
    frontier_[cell] = 0;
    for (int side = 0; side < 4; ++side) {
        const int other = neighbor(cell, side);
        if (other >= 0 && board_[other].tile == kEmpty && !frontier_[other]) {
            frontier_[other] = 1;
            frontier_cells_.push_back(other);
        }
    }
}

void Game::apply_unchecked(const Action& action) {
    if (action.kind == ActionKind::PlaceTile) {
        const int row = static_cast<int>(action.row) + kCenter;
        const int column = static_cast<int>(action.column) + kCenter;
        const int cell = index(row, column);
        board_[cell].tile = current_tile_;
        board_[cell].rotation = action.rotation & 3;
        last_cell_ = cell;
        ++placed_tiles_;
        add_frontier_neighbors(cell);
        phase_ = Phase::Meeple;
        return;
    }

    if (phase_ == Phase::Tile) {
        // An unplaceable tile is discarded; the same player draws again.
        draw_next_tile();
        if (phase_ == Phase::Finished) score_end_game();
        return;
    }

    if (action.kind == ActionKind::PlaceMeeple) {
        Cell& cell = board_[last_cell_];
        cell.meeple_player = current_player_;
        cell.meeple_feature = action.feature;
        cell.meeple_group = action.feature_index;
        --meeples_[current_player_];
    }
    finish_turn();
}

void Game::draw_next_tile() {
    if (deck_position_ == deck_.size()) {
        current_tile_ = kEmpty;
        phase_ = Phase::Finished;
    } else {
        current_tile_ = deck_[deck_position_++];
        phase_ = Phase::Tile;
    }
}

void Game::finish_turn() {
    score_completed_features(last_cell_);
    score_monasteries_around(last_cell_);
    current_player_ = static_cast<std::uint8_t>((current_player_ + 1) % players_);
    draw_next_tile();
    if (phase_ == Phase::Finished) score_end_game();
}

Game::FeatureResult Game::trace_feature(int start_cell, Feature feature, std::uint8_t group) const {
    FeatureResult result;
    std::array<std::uint8_t, kBoardSize * kBoardSize * 4> seen_segment{};
    std::array<std::uint8_t, kBoardSize * kBoardSize> seen_cell{};
    std::vector<int> queue;
    const auto& start_groups = feature == Feature::City ? tile_at(start_cell).city_groups
                                                        : tile_at(start_cell).road_groups;
    for (int side = 0; side < 4; ++side) {
        if (start_groups[side] == static_cast<std::int8_t>(group)) queue.push_back(start_cell * 4 + side);
    }

    for (std::size_t cursor = 0; cursor < queue.size(); ++cursor) {
        const int segment = queue[cursor];
        if (seen_segment[segment]) continue;
        seen_segment[segment] = 1;
        result.segments.push_back(segment);
        const int cell = segment / 4;
        const int side = segment & 3;
        const auto& tile = tile_at(cell);
        const auto& groups = feature == Feature::City ? tile.city_groups : tile.road_groups;
        const auto local_group = groups[side];

        // Bit 0: tile already scored. Bit 1: its meeple already counted. A feature can
        // loop back into a tile through a different segment, and the meeple may sit on
        // that later segment, so it is checked on every visit, not just the first.
        if (!(seen_cell[cell] & 1)) {
            seen_cell[cell] |= 1;
            result.cells.push_back(cell);
            result.points += 1;
            if (feature == Feature::City && tile.shield) result.points += 1;
        }
        const Cell& placed = board_[cell];
        if (!(seen_cell[cell] & 2) && placed.meeple_player != kEmpty && placed.meeple_feature == feature &&
            placed.meeple_group == static_cast<std::uint8_t>(local_group)) {
            seen_cell[cell] |= 2;
            ++result.followers[placed.meeple_player];
        }

        for (int internal = 0; internal < 4; ++internal) {
            if (groups[internal] == local_group && !seen_segment[cell * 4 + internal]) {
                queue.push_back(cell * 4 + internal);
            }
        }
        const int other = neighbor(cell, side);
        if (other < 0 || board_[other].tile == kEmpty) {
            result.complete = false;
        } else {
            queue.push_back(other * 4 + opposite(side));
        }
    }
    if (feature == Feature::City && result.complete) result.points *= 2;
    return result;
}

Game::FeatureResult Game::trace_field(int start_cell, std::uint8_t group) const {
    // Fields can sprawl over most of the board and are traced every meeple phase, so
    // visited half-edges are tracked with a per-cell generation stamp plus an 8-bit
    // mask instead of clearing a board-sized array on every call.
    thread_local std::vector<std::uint32_t> stamp(kBoardSize * kBoardSize, 0);
    thread_local std::vector<std::uint8_t> halves(kBoardSize * kBoardSize, 0);
    thread_local std::uint32_t generation = 0;
    if (++generation == 0) {
        std::fill(stamp.begin(), stamp.end(), 0);
        generation = 1;
    }
    const auto seen = [&](int cell, int half) {
        return stamp[cell] == generation && ((halves[cell] >> half) & 1);
    };

    FeatureResult result;
    result.complete = false;  // fields never complete
    std::vector<int> queue;
    const auto& start = tile_at(start_cell);
    for (int half = 0; half < 8; ++half) {
        if (start.field_groups[half] == static_cast<std::int8_t>(group)) {
            queue.push_back(start_cell * 8 + half);
            break;
        }
    }

    for (std::size_t cursor = 0; cursor < queue.size(); ++cursor) {
        const int cell = queue[cursor] / 8;
        if (seen(cell, queue[cursor] & 7)) continue;
        const auto& tile = tile_at(cell);
        const auto local_group = tile.field_groups[queue[cursor] & 7];

        result.cells.push_back(cell);
        const Cell& placed = board_[cell];
        if (placed.meeple_player != kEmpty && placed.meeple_feature == Feature::Field &&
            placed.meeple_group == static_cast<std::uint8_t>(local_group)) {
            ++result.followers[placed.meeple_player];
        }

        // Claim every half-edge of this field group on the tile at once.
        if (stamp[cell] != generation) {
            stamp[cell] = generation;
            halves[cell] = 0;
        }
        for (int half = 0; half < 8; ++half) {
            if (tile.field_groups[half] != local_group) continue;
            halves[cell] |= static_cast<std::uint8_t>(1u << half);
            result.segments.push_back(cell * 8 + half);
            const int other = neighbor(cell, half / 2);
            if (other >= 0 && board_[other].tile != kEmpty) {
                queue.push_back(other * 8 + opposite_half(half));
            }
        }
    }
    return result;
}

int Game::field_points(const FeatureResult& field) const {
    // 3 points for every distinct completed city bordering the field.
    std::vector<int> counted;  // segments of cities already evaluated
    int points = 0;
    for (int segment : field.segments) {
        const int cell = segment / 8;
        const auto& tile = tile_at(cell);
        const auto cities = tile.field_cities[tile.field_groups[segment & 7]];
        for (int side = 0; side < 4; ++side) {
            const auto city = tile.city_groups[side];
            if (city < 0 || !((cities >> city) & 1)) continue;
            if (std::find(counted.begin(), counted.end(), cell * 4 + side) != counted.end()) continue;
            const auto result = trace_feature(cell, Feature::City, static_cast<std::uint8_t>(city));
            counted.insert(counted.end(), result.segments.begin(), result.segments.end());
            if (result.complete) points += 3;
        }
    }
    return points;
}

void Game::score_feature(const FeatureResult& result) {
    const auto max_it = std::max_element(result.followers.begin(), result.followers.begin() + players_);
    if (max_it == result.followers.begin() + players_ || *max_it == 0) return;
    for (std::uint8_t player = 0; player < players_; ++player) {
        if (result.followers[player] == *max_it) scores_[player] += result.points;
    }
}

void Game::remove_feature_meeples(const FeatureResult& result, Feature feature, std::uint8_t) {
    // City/road segments are cell * 4 + side; field segments are cell * 8 + half.
    const int per_cell = feature == Feature::Field ? 8 : 4;
    for (int cell : result.cells) {
        Cell& placed = board_[cell];
        if (placed.meeple_player == kEmpty || placed.meeple_feature != feature) continue;
        bool belongs = false;
        const auto& tile = tile_at(cell);
        for (int segment : result.segments) {
            if (segment / per_cell != cell) continue;
            const int local = segment % per_cell;
            const auto group = feature == Feature::Field ? tile.field_groups[local]
                             : feature == Feature::City  ? tile.city_groups[local]
                                                         : tile.road_groups[local];
            if (group == static_cast<std::int8_t>(placed.meeple_group)) {
                belongs = true;
                break;
            }
        }
        if (belongs) {
            ++meeples_[placed.meeple_player];
            placed.meeple_player = kEmpty;
        }
    }
}

void Game::score_completed_features(int cell) {
    const auto& tile = tile_at(cell);
    for (Feature feature : {Feature::City, Feature::Road}) {
        const auto& groups = feature == Feature::City ? tile.city_groups : tile.road_groups;
        std::array<bool, 4> handled{};
        for (int side = 0; side < 4; ++side) {
            const auto group = groups[side];
            if (group < 0 || handled[group]) continue;
            handled[group] = true;
            auto result = trace_feature(cell, feature, static_cast<std::uint8_t>(group));
            if (!result.complete) continue;
            score_feature(result);
            remove_feature_meeples(result, feature, static_cast<std::uint8_t>(group));
        }
    }
}

int Game::occupied_around(int cell) const noexcept {
    const int row = cell / kBoardSize;
    const int column = cell % kBoardSize;
    int occupied = 0;
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            const int r = row + dr;
            const int c = column + dc;
            if (r >= 0 && r < kBoardSize && c >= 0 && c < kBoardSize &&
                board_[index(r, c)].tile != kEmpty) ++occupied;
        }
    }
    return occupied;
}

void Game::score_monasteries_around(int cell) {
    const int row = cell / kBoardSize;
    const int column = cell % kBoardSize;
    for (int dr = -1; dr <= 1; ++dr) {
        for (int dc = -1; dc <= 1; ++dc) {
            const int r = row + dr;
            const int c = column + dc;
            if (r < 0 || r >= kBoardSize || c < 0 || c >= kBoardSize) continue;
            Cell& placed = board_[index(r, c)];
            if (placed.tile == kEmpty || !tile_at(index(r, c)).monastery ||
                placed.meeple_player == kEmpty || placed.meeple_feature != Feature::Monastery) continue;
            if (occupied_around(index(r, c)) == 9) {
                scores_[placed.meeple_player] += 9;
                ++meeples_[placed.meeple_player];
                placed.meeple_player = kEmpty;
            }
        }
    }
}

void Game::score_end_game() {
    for (int cell = 0; cell < kBoardSize * kBoardSize; ++cell) {
        Cell& placed = board_[cell];
        if (placed.meeple_player == kEmpty) continue;
        if (placed.meeple_feature == Feature::Monastery) {
            scores_[placed.meeple_player] += occupied_around(cell);
            ++meeples_[placed.meeple_player];
            placed.meeple_player = kEmpty;
            continue;
        }
        if (placed.meeple_feature == Feature::Field) {
            auto result = trace_field(cell, placed.meeple_group);
            result.points = field_points(result);
            score_feature(result);
            remove_feature_meeples(result, Feature::Field, placed.meeple_group);
            continue;
        }
        auto result = trace_feature(cell, placed.meeple_feature, placed.meeple_group);
        score_feature(result);
        remove_feature_meeples(result, placed.meeple_feature, placed.meeple_group);
    }
}

std::uint32_t Game::play_random_game(std::uint64_t policy_seed) {
    std::mt19937_64 rng(policy_seed);
    std::vector<Action> actions;
    actions.reserve(256);
    std::uint32_t decisions = 0;
    while (!finished()) {
        legal_actions(actions);
        if (actions.empty()) throw std::logic_error("non-terminal state has no legal action");
        std::uniform_int_distribution<std::size_t> choice(0, actions.size() - 1);
        apply_unchecked(actions[choice(rng)]);
        if (++decisions > 1000) throw std::logic_error("game did not terminate");
    }
    return decisions;
}

std::uint64_t Game::hash() const noexcept {
    std::uint64_t value = 1469598103934665603ULL;
    const auto mix = [&value](std::uint64_t item) {
        value ^= item;
        value *= 1099511628211ULL;
    };
    mix(static_cast<std::uint8_t>(phase_));
    mix(farmers_);
    mix(current_player_);
    mix(current_tile_);
    mix(deck_position_);
    for (std::size_t i = deck_position_; i < deck_.size(); ++i) mix(deck_[i]);
    for (std::uint8_t player = 0; player < players_; ++player) {
        mix(static_cast<std::uint64_t>(scores_[player]));
        mix(meeples_[player]);
    }
    for (int cell = 0; cell < kBoardSize * kBoardSize; ++cell) {
        const auto& placed = board_[cell];
        if (placed.tile == kEmpty) continue;
        mix(static_cast<std::uint64_t>(cell));
        mix(placed.tile);
        mix(placed.rotation);
        mix(placed.meeple_player);
        if (placed.meeple_player != kEmpty) {
            mix(static_cast<std::uint8_t>(placed.meeple_feature));
            mix(placed.meeple_group);
        }
    }
    return value;
}

void Game::placed_tile_data(std::vector<PlacedTile>& output) const {
    output.clear();
    output.reserve(placed_tiles_);
    for (int cell = 0; cell < kBoardSize * kBoardSize; ++cell) {
        const auto& placed = board_[cell];
        if (placed.tile == kEmpty) continue;
        output.push_back({
            static_cast<std::int16_t>(cell / kBoardSize - kCenter),
            static_cast<std::int16_t>(cell % kBoardSize - kCenter),
            placed.tile,
            placed.rotation,
            placed.meeple_player,
            placed.meeple_feature,
            placed.meeple_group,
        });
    }
}

}  // namespace carcassonne
