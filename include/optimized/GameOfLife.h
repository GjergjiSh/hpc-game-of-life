#ifndef OPT_GAME_OF_LIFE_H
#define OPT_GAME_OF_LIFE_H


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
#include <ctime>
#include <string.h>
#include <sys/types.h>

typedef struct
{
    u_char* cells = nullptr;
    u_int rows;
    u_int cols;
    u_int length;
} board_t;

u_char select_cell_at(board_t& board, const u_int row, const u_int col);
void print_cell_bin_vals(u_char selcted_cell);
void spawn_gilder(board_t& board, const bool& verbose);

int cell_is_alive(board_t& board, u_int col_idx, u_int row_idx);
void get_wrap_indexes(board_t& board, const u_int& cell_x, const u_int& cell_y, int& xleft, int& xright, int& yabove, int& ybelow);
void spawn_cell(board_t& board, const u_int& row, const u_int& col);
void kill_cell(board_t& board, const u_int& row, const u_int& col);

void generate_initial_board_state(board_t& board, const float live_cell_percentage);
void generate_next_board_state(board_t& board, board_t& temp_board);

void display_board_state(board_t& board, const bool& verbose);
void game_of_life_loop(board_t& board, board_t& temp_board, const int& generations, const int& display, const bool& verbose);


#endif /* OPT_GAME_OF_LIFE_H */