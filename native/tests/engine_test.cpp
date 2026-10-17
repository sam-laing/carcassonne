#include "carcassonne/engine.hpp"

#include <cstdint>
#include <iostream>
#include <numeric>
#include <stdexcept>

using namespace carcassonne;

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main() {
    const auto& catalog = Game::tile_catalog();
    const int tile_count = std::accumulate(catalog.begin(), catalog.end(), 0,
        [](int sum, const TileDefinition& tile) { return sum + tile.count; });
    require(catalog.size() == 32, "catalog must contain 32 tile types");
    require(tile_count == 72, "base deck must contain 72 tiles");

    Game game(42, 2);
    auto actions = game.legal_actions();
    require(actions.size() == 1, "initial state must have one normalized placement");
    require(actions[0].kind == ActionKind::PlaceTile, "initial action must place a tile");
    require(actions[0].row == 0 && actions[0].column == 0, "initial tile must use the origin");
    require(game.apply(actions[0]), "initial placement was rejected");
    require(game.phase() == Phase::Meeple, "placement must enter the meeple phase");
    require(game.placed_tiles() == 1, "placed tile count was not updated");

    Game clone = game;
    require(clone.hash() == game.hash(), "cloned state hash differs");
    actions = clone.legal_actions();
    require(!actions.empty(), "meeple phase has no legal action");
    require(clone.apply(actions.back()), "legal pass was rejected");
    require(clone.hash() != game.hash(), "transition did not alter cloned state");

    std::uint64_t total_decisions = 0;
    for (std::uint64_t seed = 0; seed < 100; ++seed) {
        Game simulation(seed, 2);
        const auto decisions = simulation.play_random_game(seed + 10000);
        require(simulation.finished(), "random game did not finish");
        require(simulation.placed_tiles() <= 72, "more tiles placed than exist in deck");
        require(decisions >= simulation.placed_tiles(), "invalid decision count");
        require(simulation.meeples(0) == 7, "player 0 meeples were not returned");
        require(simulation.meeples(1) == 7, "player 1 meeples were not returned");
        require(simulation.score(0) >= 0 && simulation.score(1) >= 0, "negative score");
        total_decisions += decisions;
    }

    // Field placements are offered only with farmers enabled, and every farmer is
    // returned when the game is scored.
    std::uint64_t farmer_placements = 0;
    for (std::uint64_t seed = 0; seed < 50; ++seed) {
        for (bool farmers : {true, false}) {
            Game simulation(seed, 2, farmers);
            std::mt19937_64 rng(seed);
            while (!simulation.finished()) {
                const auto options = simulation.legal_actions();
                for (const auto& option : options) {
                    require(farmers || option.feature != Feature::Field || option.kind != ActionKind::PlaceMeeple,
                            "field placement offered with farmers disabled");
                }
                const auto& action = options[rng() % options.size()];
                if (action.kind == ActionKind::PlaceMeeple && action.feature == Feature::Field) ++farmer_placements;
                simulation.apply_unchecked(action);
            }
            require(simulation.meeples(0) == 7 && simulation.meeples(1) == 7, "farmers were not returned");
        }
    }
    require(farmer_placements > 0, "no farmer was ever placed");

    Game deterministic_a(1234, 2);
    Game deterministic_b(1234, 2);
    require(deterministic_a.play_random_game(5678) == deterministic_b.play_random_game(5678),
            "seeded decision counts differ");
    require(deterministic_a.hash() == deterministic_b.hash(), "seeded games are not deterministic");
    std::cout << "native engine tests passed; 100 games, " << total_decisions
              << " decisions\n";
}
