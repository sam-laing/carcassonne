#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <random>
#include <string_view>
#include <vector>

namespace carcassonne {

enum class Terrain : std::uint8_t { Grass, City, Road };
enum class Phase : std::uint8_t { Tile, Meeple, Finished };
enum class ActionKind : std::uint8_t { PlaceTile, PlaceMeeple, Pass };
enum class Feature : std::uint8_t { City, Road, Monastery, Field };

struct Action {
    ActionKind kind{ActionKind::Pass};
    std::int16_t row{0};
    std::int16_t column{0};
    std::uint8_t rotation{0};
    Feature feature{Feature::City};
    std::uint8_t feature_index{0};

    bool operator==(const Action& other) const noexcept {
        return kind == other.kind && row == other.row && column == other.column &&
               rotation == other.rotation && feature == other.feature &&
               feature_index == other.feature_index;
    }
};

struct TileDefinition {
    std::string_view name;
    std::uint8_t count;
    std::array<Terrain, 4> edges;
    std::array<std::int8_t, 4> city_groups;
    std::array<std::int8_t, 4> road_groups;
    bool shield;
    bool monastery;
};

struct PlacedTile {
    std::int16_t row;
    std::int16_t column;
    std::uint8_t tile;
    std::uint8_t rotation;
    std::uint8_t meeple_player;
    Feature meeple_feature;
    std::uint8_t meeple_group;
};

class Game {
public:
    static constexpr int kBoardSize = 151;
    static constexpr int kCenter = kBoardSize / 2;
    static constexpr int kMaxPlayers = 6;
    static constexpr std::uint8_t kEmpty = 255;

    explicit Game(std::uint64_t seed = 0, std::uint8_t players = 2, bool farmers = true);

    void reset(std::uint64_t seed);
    [[nodiscard]] std::vector<Action> legal_actions() const;
    void legal_actions(std::vector<Action>& output) const;
    [[nodiscard]] bool apply(const Action& action);
    void apply_unchecked(const Action& action);
    [[nodiscard]] std::uint32_t play_random_game(std::uint64_t policy_seed);

    [[nodiscard]] Phase phase() const noexcept { return phase_; }
    [[nodiscard]] bool finished() const noexcept { return phase_ == Phase::Finished; }
    [[nodiscard]] std::uint8_t current_player() const noexcept { return current_player_; }
    [[nodiscard]] std::uint8_t players() const noexcept { return players_; }
    [[nodiscard]] bool farmers() const noexcept { return farmers_; }
    [[nodiscard]] std::uint8_t current_tile() const noexcept { return current_tile_; }
    [[nodiscard]] std::uint16_t placed_tiles() const noexcept { return placed_tiles_; }
    [[nodiscard]] std::uint16_t remaining_tiles() const noexcept;
    [[nodiscard]] int score(std::uint8_t player) const noexcept { return scores_[player]; }
    [[nodiscard]] std::uint8_t meeples(std::uint8_t player) const noexcept { return meeples_[player]; }
    [[nodiscard]] std::uint64_t hash() const noexcept;
    void placed_tile_data(std::vector<PlacedTile>& output) const;

    [[nodiscard]] static const std::array<TileDefinition, 32>& tile_catalog();
    [[nodiscard]] static std::size_t deck_size() noexcept { return 72; }

private:
    struct RotatedTile {
        std::array<Terrain, 4> edges{};
        std::array<std::int8_t, 4> city_groups{};
        std::array<std::int8_t, 4> road_groups{};
        std::array<std::int8_t, 8> field_groups{};
        std::array<std::uint8_t, 4> field_cities{};
        bool shield{false};
        bool monastery{false};
    };

    struct Cell {
        std::uint8_t tile{kEmpty};
        std::uint8_t rotation{0};
        std::uint8_t meeple_player{kEmpty};
        Feature meeple_feature{Feature::City};
        std::uint8_t meeple_group{0};
    };

    struct FeatureResult {
        bool complete{true};
        int points{0};
        std::array<std::uint8_t, kMaxPlayers> followers{};
        std::vector<int> cells;
        std::vector<int> segments;
    };

    static const std::array<std::array<RotatedTile, 4>, 32>& rotated_catalog();
    [[nodiscard]] static int index(int row, int column) noexcept { return row * kBoardSize + column; }
    [[nodiscard]] static int opposite(int side) noexcept { return (side + 2) & 3; }
    // The half-edge a field crosses into on the neighbouring tile.
    [[nodiscard]] static int opposite_half(int half) noexcept { return ((half + 4) & 7) ^ 1; }
    [[nodiscard]] static int neighbor(int cell, int side) noexcept;
    [[nodiscard]] const RotatedTile& tile_at(int cell) const noexcept;
    [[nodiscard]] bool tile_fits(int cell, std::uint8_t rotation) const noexcept;
    [[nodiscard]] FeatureResult trace_feature(int start_cell, Feature feature, std::uint8_t group) const;
    [[nodiscard]] FeatureResult trace_field(int start_cell, std::uint8_t group) const;
    [[nodiscard]] int field_points(const FeatureResult& field) const;
    [[nodiscard]] int occupied_around(int cell) const noexcept;
    void add_frontier_neighbors(int cell);
    void finish_turn();
    void draw_next_tile();
    void score_completed_features(int cell);
    void score_monasteries_around(int cell);
    void score_feature(const FeatureResult& result);
    void remove_feature_meeples(const FeatureResult& result, Feature feature, std::uint8_t group);
    void score_end_game();

    std::array<Cell, kBoardSize * kBoardSize> board_{};
    std::array<std::uint8_t, kBoardSize * kBoardSize> frontier_{};
    std::vector<int> frontier_cells_;
    std::array<std::uint8_t, 72> deck_{};
    std::size_t deck_position_{0};
    std::array<int, kMaxPlayers> scores_{};
    std::array<std::uint8_t, kMaxPlayers> meeples_{};
    std::uint8_t players_{2};
    bool farmers_{true};
    std::uint8_t current_player_{0};
    std::uint8_t current_tile_{kEmpty};
    Phase phase_{Phase::Tile};
    std::uint16_t placed_tiles_{0};
    int last_cell_{-1};
};

}  // namespace carcassonne
