#include "unoptimized/GameOfLife.h"
#include "common/Patterns.h"
#include "common/Timing.h"
#include <iostream>

int main(int argc, char* argv[])
{
    //Board at generation n-1
    board_t board;
    board.row_nr = std::stoi(argv[1]);
    board.col_nr = std::stoi(argv[2]);

    board.cell_rows = GLIDER;
    //generate_initial_board_state(previous, std::stoi(argv[1]), std::stoi(argv[2]));

    int generations = std::stoi(argv[3]);
    int display = std::stoi(argv[4]);

    if (display) {
        std::cout << "Initial Board" << std::endl;
        display_board_state(board);
    }

    TimedScope unoptimized_gol_timer("GoL timer");
    game_of_life_loop(board, generations, display);

    return 0;
}