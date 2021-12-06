#include "OptGameOfLife.h"

#include <string.h>
#include <iostream>
#include <ctime>

// @9SMTM6 To avoid confusion:
// there are two ways of thinking about indexing a 1d array as a 2d array. (Explained in docs)
// cell_is_alive and select_cell_at respectively use one of each method to illustrate
// I prefer row, col where row is the number of the row and col is the number of the column

int cell_is_alive(board_t& board, u_int col_idx, u_int row_idx)
{
    //std::cout << "Selecting cell at " << col_idx << "|" << row_idx << std::endl;

    // Indexing a 1d array as a 2d array by using the col and row indexes of the desired element
    u_char* cell_ptr = board.cells + (row_idx * board.cols) + col_idx;

    return *cell_ptr & 0x01; // return the first bit of the cell
}

u_char select_cell_at(board_t& board, const u_int row, const u_int col)
{

    std::cout << "Selecting cell at " << row << "|" << col << std::endl;

    // Indexing a 1d array as a 2d array by using the row and column numbers
    u_char* cell_ptr = board.cells + (col * board.cols) + row;
    return *cell_ptr;
}


// @9SMTM6 #TODO Read somewhere that using conditionals is faster than using the modulo in the unopt version
// Have yet to isolate the difference and test it out myself
void get_wrap_indexes(board_t& board, const u_int& cell_x, const u_int& cell_y, int& x_left, int& x_right, int& y_above, int& y_below)
{

    if (cell_x == 0) // left edge of the board
        x_left =  board.cols - 1;
    else
        x_left = -1;

    if (cell_x == ( board.cols - 1)) // right edge of the board
        x_right = -( board.cols - 1);
    else
        x_right = 1;

    if (cell_y == 0) // top edge of the board
        y_above =  board.length -  board.cols;
    else
        y_above = - board.cols;

    if (cell_y == ( board.rows - 1)) // bottom edge of the board
        y_below = -( board.length -  board.cols);
    else
        y_below =  board.cols;
}

// "Activate" the cell by setting its first bit to 1 and increment all of its 8 neighbours living cell counts by one
void spawn_cell(board_t& board,const u_int& row, const u_int& col)
{
    // index 1d array as 2d array
    u_char* cell_ptr = board.cells + (col * board.cols) + row;

    // make cell alive by setting first bit to 1
    *(cell_ptr) |= 0x01;

    // Handle indexes if the cell is on the edges of the board
    int x_left, x_right, y_above, y_below;
    get_wrap_indexes(board, row, col, x_left, x_right, y_above, y_below);

    // If the cell is alive, each surrounding cell has it's neighbour count incremented
    // @9SMTM6 we add 2 because we are starting at the 1st bit not the 0th bit
    // Remember the 0th bit represents the state of the cell.

    //[1][2][3]
    //[4][ ][5]
    //[6][7][8]

    *(cell_ptr + y_above + x_left) += 0x02;  //[1]
    *(cell_ptr + y_above) += 0x02;           //[2]
    *(cell_ptr + y_above + x_right) += 0x02; //[3]
    *(cell_ptr + x_left) += 0x02;            //[4]
    *(cell_ptr + x_right) += 0x02;           //[5]
    *(cell_ptr + y_below + x_left) += 0x02;  //[6]
    *(cell_ptr + y_below) += 0x02;           //[7]
    *(cell_ptr + y_below + x_right) += 0x02; //[8]
}

// "Kill" the cell by setting its first bit to 0 and deincrement all of its 8 neighbours living cell counts by one
void kill_cell(board_t& board, const u_int& row, const u_int& col)
{
    // index 1d array as 2d array
    u_char* cell_ptr = board.cells + (col * board.cols) + row;

     // kill the cell by setting first bit to 0
    *(cell_ptr) &= ~0x01;

    // Handle indexes if the cell is on the edges of the board
    int x_left, x_right, y_above, y_below;
    get_wrap_indexes(board, row, col, x_left, x_right, y_above, y_below);

    // Deincrement the neighbour count of all surrounding cells since the cell died
    *(cell_ptr + y_above + x_left) -= 0x02;
    *(cell_ptr + y_above) -= 0x02;
    *(cell_ptr + y_above + x_right) -= 0x02;
    *(cell_ptr + x_left) -= 0x02;
    *(cell_ptr + x_right) -= 0x02;
    *(cell_ptr + y_below + x_left) -= 0x02;
    *(cell_ptr + y_below) -= 0x02;
    *(cell_ptr + y_below + x_right) -= 0x02;
}


void generate_initial_state(board_t& board, const float live_cell_percentage)
{
    u_int seed = (unsigned)time(NULL);
    srand(seed);

    u_int x, y;

    for (int i = 0; i < board.length * live_cell_percentage; i++) {
        x = rand() % (board.cols - 1); // rand nr between 0 and cols - 1
        y = rand() % (board.rows - 1); // rand nr between 0 and length - 1

        // @9SMTM6 we pick at random so we need to make sure a cell is not set alive twice
        // by checking its current state
        if (!cell_is_alive(board, x, y)) {
            spawn_cell(board, x, y);
        }
    }
}

//@GjergjiSh #TODO clear up indexing here to make the code a bit more readable
void generate_next_board_state(board_t& board, board_t& temp_board)
{
    u_int row, col, nb_count;
    u_char* cell_ptr;

    memcpy(temp_board.cells, board.cells, board.length);

    cell_ptr = temp_board.cells;

    for (row = 0; row < board.rows; row++) {
        col = 0;
        do {

            // Skip all dead cells with no neighbours
            // Cells with no neighbours are 0 bytes
            while (*cell_ptr == 0) {
                cell_ptr++;
                if (++col >= board.cols)
                    goto SKIP_ROW;
            }

            // Get the neighbour count by shifting the bits in the cell by one to the left
            // this removes the state bit and only leaves the nb count bits
            nb_count = *cell_ptr >> 1;

            if (*cell_ptr & 0x01) { // if the first bit of the selected cell is 1
                if ((nb_count != 2) && (nb_count != 3)) {
                    kill_cell(board, col, row);
                }
            } else {
                if (nb_count == 3) {
                    spawn_cell(board, col, row);
                }
            }

            cell_ptr++;

        } while (++col < board.cols);
    SKIP_ROW:;
    }
}

void display_board_state(board_t& board, const bool& verbose)
{
    for (u_int row = 0; row < board.rows; row++) {
        std::cout << "|";
        for (u_int col = 0; col < board.cols; col++) {
            if (cell_is_alive(board, row,col)) {
                std::cout << "*";
            } else {
                std::cout << " ";
            }
        }
        std::cout << "|" << std::endl;
    }

    std::cout << "\n";

    if (verbose) {
        for (int row = 0; row < board.rows; row++) {
            for (int col = 0; col < board.cols; col++) {
                if (cell_is_alive(board, row, col)) {

                    u_char* cell_ptr = board.cells + (col * board.cols) + row;
                    int nb_cnt = *cell_ptr >> 1;
                    std::cout << "Cell at :" << row << "|" << col << " is alive and has "
                              << nb_cnt << " neighbours"
                              << std::endl;
                }
            }
        }
    }
}

//Generate a set of iterations
void game_of_life_loop(board_t& board, board_t& temp_board, const int& generations, const int& display, const bool& verbose)
{
    for (int gen = 0; gen < generations; gen++) {
        generate_next_board_state(board, temp_board);

        if (display) {
            std::cout << "State at generation n=" << gen << std::endl;
            display_board_state(board, verbose);
        }
    }
}

// Helper method to check the values of the cell
void print_binary_val(u_char selected_cell)
{
    const u_char temp = selected_cell;
    int binary[8];
    for (int n = 0; n < 8; n++) {
        binary[7-n] = (temp >> n) & 1;
    }

    for (int n = 0; n < 8; n++) {
        if (n == 3) {
            printf("%d ", binary[n]);
        } else {
            printf("%d", binary[n]);
        }
    }
    printf("\n");
}

// Helper method to intialize the board with a glider pattern
void spawn_gilder(board_t& board, const bool& verbose)
{
    spawn_cell(board, 1, 2);
    spawn_cell(board, 2, 3);
    spawn_cell(board, 3, 1);
    spawn_cell(board, 3, 2);
    spawn_cell(board, 3, 3);

    if (verbose) {
        std::cout << "Glider Vals" << std::endl;
        print_binary_val(select_cell_at(board, 1,2));
        print_binary_val(select_cell_at(board, 2,3));
        print_binary_val(select_cell_at(board, 3,1));
        print_binary_val(select_cell_at(board, 3,2));
        print_binary_val(select_cell_at(board, 3,3));
    }
}