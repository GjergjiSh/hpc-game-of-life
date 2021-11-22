#include "OptGameOfLife.h"

// Generates an initial board state with size (rows:cols)
void generate_initial_board_state(board_t& board, int rows, int cols)
{
    board.row_nr = rows;
    board.col_nr = cols;

    for (int row = 0; row < board.row_nr; row++) {

        std::vector<int> cell_row;

        for (int col = 0; col < board.col_nr; col++) {
            int val = rand() % 2;
            cell_row.push_back(val);
        }

        board.cell_rows.push_back(cell_row);
    }
}

// Select the cell at (row:col) of the board
int cell_state_at(int row, int col, board_t& board)
{
    // use % to get wrapping effect at board edges
    return board.cell_rows.at((row + board.row_nr) % board.row_nr).at((col + board.col_nr) % board.col_nr);
}

// Get the neighbour count of the cell at (row:col) of the board
int get_neighbour_count(int row, int col, board_t& board)
{
    int neighbour_count = 0;
    std::vector<int> indexes { -1, 0, 1 };

    // Check col
    for (int i : indexes) {

        // Check Row
        for (int j : indexes) {

            // Avoid the current cell
            if (j || i) {

                // Add cell state (0|1) to neighbour count
                neighbour_count += cell_state_at(row + j, col + i, board);
            }
        }
    }

    return neighbour_count;
}

// Print out the value of a cell (* for ALIVE " " for DEAD)
void display_cell_state(int cell)
{
    std::cout << (cell ? "*" : " ");
}

// Displays cell state for an entire row
void display_row_state(const std::vector<int>& row)
{
    std::cout << "|";
    // Loop through cells in row and display their values
    for (int cell : row) {
        display_cell_state(cell);
    }
    std::cout << "|" << std::endl;
}

// Displays all the cell values for the board
void display_board_state(board_t& board)
{
    for (const std::vector<int>& row : board.cell_rows) {
        display_row_state(row);
    }
    //Sleep for 1s after displaying the state and clear stdout
    //sleep(1);
    //int ret = std::system("clear");
}

// Generates the next state of a cell based on the rules of the game
int generate_cell_state(int row, int col, board_t& board)
{
    int cell_neighbour_count = get_neighbour_count(row, col, board);
    int cell_survives = board.cell_rows.at(row).at(col) && (cell_neighbour_count == 2 || cell_neighbour_count == 3);
    int cell_birth = !board.cell_rows.at(row).at(col) && (cell_neighbour_count == 3);
    return cell_survives || cell_birth;
}

// Loops through all board cells and generates the next board state based on the rules of the game
void generate_next_board_state(board_t& current_board_state)
{
    //Store current board state to avoid overriding during cell state generation
    board_t temp = current_board_state;

    // Loop through rows
    for (int row = 0; row < current_board_state.row_nr; row++) {

        // Loop through cols
        for (int col = 0; col < current_board_state.col_nr; col++) {

            // generate new states for row
            current_board_state.cell_rows.at(row).at(col) = generate_cell_state(row, col, temp);
        }
    }
}

//Generate a set of iterations
void game_of_life_loop(board_t& board, int generations, int display)
{
    for (int gen = 0; gen < generations; gen++) {
        generate_next_board_state(board);

        if (display) {
            std::cout << "State at generation n=" << gen << std::endl;
            display_board_state(board);
        }
    }
}
