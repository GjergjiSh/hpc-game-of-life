#include "GameOfLife.h"
#include "Patterns.h"
#include <iostream>

int main(int argc, char* argv[])
{
    std::cout << "Unoptimized Game of Life" << std::endl;

    //Board at generation n-1
    board_t board;
    board.col_nr = 20;
    board.row_nr = 10;

    board.cell_rows = GLIDER;
    //generate_initial_board_state(previous, std::stoi(argv[1]), std::stoi(argv[2]));

    int generations = std::stoi(argv[3]);
    int display = std::stoi(argv[4]);

    if (display) {
        std::cout << "Initial Board" << std::endl;
        display_board_state(board);
    }

    game_of_life_loop(board, generations, display);

    return 0;
}