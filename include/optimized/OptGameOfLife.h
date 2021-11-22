/*

Rules of the game:

- Any live cell with fewer than two live neighbours dies, as if by underpopulation.
- Any live cell with two or three live neighbours lives on to the next generation.
- Any live cell with more than three live neighbours dies, as if by overpopulation.
- Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.

These rules, which compare the behavior of the automaton to real life, can be condensed into the following:

- Any live cell with two or three live neighbours survives.
- Any dead cell with three live neighbours becomes a live cell.
- All other live cells die in the next generation. Similarly, all other dead cells stay dead.

*/

#include <iostream>
#include <vector>
#include <numeric>
#include <unistd.h>

typedef struct
{
    std::vector<std::vector<int>> cell_rows;
    int row_nr;
    int col_nr;
} board_t;


void generate_initial_board_state(board_t& board, int rows, int cols);

int cell_state_at(int row, int col, board_t& board);
int get_neighbour_count(int row, int col, board_t& board);
int generate_cell_state(int row, int col, board_t& board);
void generate_next_board_state(board_t& board);

void display_cell_state(int cell);
void display_board_state(board_t& board);
void game_of_life_loop(board_t& board, int generations, int display);