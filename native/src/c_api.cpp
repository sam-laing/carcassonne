#include "carcassonne/c_api.h"
#include "carcassonne/engine.hpp"

#include <algorithm>
#include <new>
#include <vector>

struct carcassonne_game {
    carcassonne::Game value;
};

namespace {
carcassonne_action to_c(const carcassonne::Action& action) {
    return {static_cast<uint8_t>(action.kind), action.row, action.column, action.rotation,
            static_cast<uint8_t>(action.feature), action.feature_index};
}

carcassonne::Action from_c(carcassonne_action action) {
    return {static_cast<carcassonne::ActionKind>(action.kind), action.row, action.column,
            action.rotation, static_cast<carcassonne::Feature>(action.feature), action.feature_index};
}

carcassonne_placed_tile to_c(const carcassonne::PlacedTile& tile) {
    return {tile.row, tile.column, tile.tile, tile.rotation, tile.meeple_player,
            static_cast<uint8_t>(tile.meeple_feature), tile.meeple_group};
}
}  // namespace

extern "C" {

carcassonne_game* carcassonne_game_new(uint64_t seed, uint8_t players) {
    try { return new carcassonne_game{carcassonne::Game(seed, players)}; }
    catch (...) { return nullptr; }
}

carcassonne_game* carcassonne_game_new_with_rules(uint64_t seed, uint8_t players, uint8_t farmers) {
    try { return new carcassonne_game{carcassonne::Game(seed, players, farmers != 0)}; }
    catch (...) { return nullptr; }
}

carcassonne_game* carcassonne_game_clone(const carcassonne_game* game) {
    if (!game) return nullptr;
    try { return new carcassonne_game{game->value}; }
    catch (...) { return nullptr; }
}

void carcassonne_game_free(carcassonne_game* game) { delete game; }

void carcassonne_game_reset(carcassonne_game* game, uint64_t seed) {
    if (game) game->value.reset(seed);
}

size_t carcassonne_legal_actions(const carcassonne_game* game, carcassonne_action* output, size_t capacity) {
    if (!game) return 0;
    const auto actions = game->value.legal_actions();
    if (output) {
        const size_t count = std::min(capacity, actions.size());
        for (size_t i = 0; i < count; ++i) output[i] = to_c(actions[i]);
    }
    return actions.size();
}

int carcassonne_apply(carcassonne_game* game, carcassonne_action action) {
    return game && game->value.apply(from_c(action)) ? 1 : 0;
}

uint32_t carcassonne_play_random_game(carcassonne_game* game, uint64_t policy_seed) {
    if (!game) return 0;
    try { return game->value.play_random_game(policy_seed); }
    catch (...) { return 0; }
}

uint8_t carcassonne_phase(const carcassonne_game* game) {
    return game ? static_cast<uint8_t>(game->value.phase()) : 2;
}
uint8_t carcassonne_players(const carcassonne_game* game) {
    return game ? game->value.players() : 0;
}
uint8_t carcassonne_current_player(const carcassonne_game* game) {
    return game ? game->value.current_player() : 0;
}
uint8_t carcassonne_current_tile(const carcassonne_game* game) {
    return game ? game->value.current_tile() : carcassonne::Game::kEmpty;
}
uint16_t carcassonne_placed_tiles(const carcassonne_game* game) {
    return game ? game->value.placed_tiles() : 0;
}
uint16_t carcassonne_remaining_tiles(const carcassonne_game* game) {
    return game ? game->value.remaining_tiles() : 0;
}
size_t carcassonne_board(const carcassonne_game* game, carcassonne_placed_tile* output, size_t capacity) {
    if (!game) return 0;
    std::vector<carcassonne::PlacedTile> tiles;
    game->value.placed_tile_data(tiles);
    if (output) {
        const size_t count = std::min(capacity, tiles.size());
        for (size_t i = 0; i < count; ++i) output[i] = to_c(tiles[i]);
    }
    return tiles.size();
}
int carcassonne_score(const carcassonne_game* game, uint8_t player) {
    return game && player < game->value.players() ? game->value.score(player) : 0;
}
uint8_t carcassonne_meeples(const carcassonne_game* game, uint8_t player) {
    return game && player < game->value.players() ? game->value.meeples(player) : 0;
}
uint64_t carcassonne_hash(const carcassonne_game* game) {
    return game ? game->value.hash() : 0;
}
size_t carcassonne_tile_type_count(void) { return carcassonne::Game::tile_catalog().size(); }
const char* carcassonne_tile_name(uint8_t tile) {
    const auto& catalog = carcassonne::Game::tile_catalog();
    return tile < catalog.size() ? catalog[tile].name.data() : nullptr;
}

}  // extern "C"
