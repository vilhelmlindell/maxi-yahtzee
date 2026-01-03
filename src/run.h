#include "game.h"

struct Config {
    int games = 1000;
    int ms_per_move = 10;
    int threads = 8;
    bool debug = false;
};

void run_args(int argc, char* argv[]);
void run_games(Config config);
void run_game(Game& game, Config config);
Move run_mcts(Game& game, Config config);
