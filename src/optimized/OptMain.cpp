#include "OptGameOfLife.h"
#include "OptPatterns.h"
#include "OptTiming.h"
#include <iostream>

int main(int argc, char* argv[])
{
    std::cout << "Optimized Game of Life" << std::endl;

    const bool verbose = 0;

    // @9SMTM6 Weird undefined behaviour when the height and width of the cell array are non symmetrical
    // Still needs to be figured out
    board_t board;
    board.rows = std::stoi(argv[1]);
    board.cols = std::stoi(argv[2]);
    board.length = board.rows * board.rows;
    board.cells = new unsigned char[board.length];

    // Current state of the board (cells) is copied into the temp_board at next state generation
    board_t temp_board;
    temp_board.rows = std::stoi(argv[1]);
    temp_board.cols = std::stoi(argv[2]);
    temp_board.length = temp_board.rows * temp_board.rows;
    temp_board.cells = new unsigned char[temp_board.length];

    size_t single_cell_size = sizeof(*board.cells);
    size_t cell_array_size = board.length * single_cell_size;
    std::cout << "Size of each cell is " << single_cell_size << "bytes" << std::endl;
    std::cout << "Size of the entire array is " <<  cell_array_size << "bytes" << std::endl;

    //Initialize everything to 0
    memset(board.cells, 0, cell_array_size);

    // Spawn glider pattern for testing purpouses
    spawn_gilder(board, verbose);

    int generations = std::stoi(argv[3]);
    int display = std::stoi(argv[4]);

    std::cout << "Generating " << generations << " iterations of Game of Life" << std::endl;

    TimedScope optimized_gol_timer("Optimized game of life timer");
    game_of_life_loop(board, temp_board, generations, display, verbose);

    return 0;
}