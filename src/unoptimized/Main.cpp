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

    spawn_glider(board);
    //generate_initial_board_state(previous, std::stoi(argv[1]), std::stoi(argv[2]));

    board_t temp;
    temp.row_nr = board.row_nr;
    temp.col_nr = board.col_nr;
    temp.cell_rows = board.cell_rows;

    int generations = std::stoi(argv[3]);
    int display = std::stoi(argv[4]);

    if (display) {
        std::cout << "Initial Board" << std::endl;
        display_board_state(board);
    }

    TimedScope unoptimized_gol_timer("Unoptimized GoL timer");

    // 5th parameter = 1 => Serial execution, 5th parameter = 0 => Parallel execution
    if (std::stoi(argv[5]) == 1) {
        game_of_life_loop(board, generations, display);
    } else {
        game_of_life_loop_omp(board, temp, generations, display);
    }

    return 0;
}