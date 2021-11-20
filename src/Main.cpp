#include "GameOfLife.h"
#include "Patterns.h"
#include <iostream>

int main(int argc, char* argv[])
{

    board_t board;

    //board.col_nr = 20;
    //board.row_nr = 10;
    //board.cell_rows = GLIDER;

    generate_initial_board_state(board, std::stoi(argv[1]), std::stoi(argv[2]));
    int generations = std::stoi(argv[3]);

    for (int gen = 0; gen < generations; gen++) {
        std::cout << "State at generation: " << gen << std::endl;
        display_board_state(board);
        generate_next_board_state(board);

    }

    return 0;
}