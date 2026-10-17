#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct carcassonne_game carcassonne_game;

typedef struct carcassonne_action {
    uint8_t kind;
    int16_t row;
    int16_t column;
    uint8_t rotation;
    uint8_t feature;
    uint8_t feature_index;
} carcassonne_action;

typedef struct carcassonne_placed_tile {
    int16_t row;
    int16_t column;
    uint8_t tile;
    uint8_t rotation;
    uint8_t meeple_player;
    uint8_t meeple_feature;
    uint8_t meeple_group;
} carcassonne_placed_tile;

carcassonne_game* carcassonne_game_new(uint64_t seed, uint8_t players);
carcassonne_game* carcassonne_game_new_with_rules(uint64_t seed, uint8_t players, uint8_t farmers);
carcassonne_game* carcassonne_game_clone(const carcassonne_game* game);
void carcassonne_game_free(carcassonne_game* game);
void carcassonne_game_reset(carcassonne_game* game, uint64_t seed);

size_t carcassonne_legal_actions(const carcassonne_game* game, carcassonne_action* output, size_t capacity);
int carcassonne_apply(carcassonne_game* game, carcassonne_action action);
uint32_t carcassonne_play_random_game(carcassonne_game* game, uint64_t policy_seed);

uint8_t carcassonne_phase(const carcassonne_game* game);
uint8_t carcassonne_players(const carcassonne_game* game);
uint8_t carcassonne_current_player(const carcassonne_game* game);
uint8_t carcassonne_current_tile(const carcassonne_game* game);
uint16_t carcassonne_placed_tiles(const carcassonne_game* game);
uint16_t carcassonne_remaining_tiles(const carcassonne_game* game);
size_t carcassonne_board(const carcassonne_game* game, carcassonne_placed_tile* output, size_t capacity);
int carcassonne_score(const carcassonne_game* game, uint8_t player);
uint8_t carcassonne_meeples(const carcassonne_game* game, uint8_t player);
uint64_t carcassonne_hash(const carcassonne_game* game);
size_t carcassonne_tile_type_count(void);
const char* carcassonne_tile_name(uint8_t tile);

#ifdef __cplusplus
}
#endif
