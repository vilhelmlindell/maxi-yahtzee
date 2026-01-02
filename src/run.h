#include "game.h"

void run_interface();
void run_games(int game_count, int ms_per_move, int thread_count);
void run_game(Game& game, int ms_per_move, int thread_count);
Move run_mcts(Game& game, int ms_per_move, int thread_count);
