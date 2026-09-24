#include "carcassonne/engine.hpp"

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    const std::uint64_t games = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 10000;
    std::uint64_t decisions = 0;
    const auto start = std::chrono::steady_clock::now();
    for (std::uint64_t seed = 0; seed < games; ++seed) {
        carcassonne::Game game(seed, 2);
        decisions += game.play_random_game(seed ^ 0x9e3779b97f4a7c15ULL);
    }
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << std::fixed << std::setprecision(2)
              << games << " games in " << seconds << " s: "
              << games / seconds << " games/s, " << decisions / seconds
              << " decisions/s\n";
}
