#include <exception>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <tuple>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <memory>
#include <vector>
#include <iostream>
#include "common/Timing.h"
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#endif

#define MAX_SELECTIONS 10
#define MAX_NAME_LENGTH 150

void display_board_state(bool * board, uint board_width, uint board_height);

#define BLOCK_SIZE 8

/// fixes modulo for the purposes of this file (maximally negative the modulo range). Also may whoever decided Cs modulo behavior was right like it is suffer...
int fixed_modulo(int val, int mod) {
    return (val + mod) % mod;
}

#define LOCAL_MEM_BLOCK_SIZE (BLOCK_SIZE + 2)

void game_of_life_split_util_local_mem(
    int x_width,
    int y_width,
    bool* board,
    bool* group_board, // has size (BLOCK_SIZE + 2)²
    int y_local,
    int x_local,
    int y_group,
    int x_group
)
{
    // "true" x and y position on the board (not bloated by larger kernel workgroup size for caching, wrapped around)
    int y = fixed_modulo(y_group * BLOCK_SIZE + (y_local - 1), y_width);
    int x = fixed_modulo(x_group * BLOCK_SIZE + (x_local - 1), x_width);

    bool was_alive = board[y * x_width + x];
    group_board[y_local * LOCAL_MEM_BLOCK_SIZE + x_local] = was_alive;
}

void game_of_life_split_util_global_mem(
    int x_width,
    int y_width,
    bool* board,
    bool* group_board, // has size (BLOCK_SIZE + 2)²
    int y_local,
    int x_local,
    int y_group,
    int x_group
) {
    bool was_alive = group_board[y_local * LOCAL_MEM_BLOCK_SIZE + x_local];
    // DISABLED
    // group_board[y_local * LOCAL_MEM_BLOCK_SIZE + x_local] = was_alive;

    // if this is a cell at the border its only real purpose was to write the local memory. I left the other stuff there because it will still realistically be executed like this
    bool is_compute_cell = !((y_local == 0) | (y_local == (BLOCK_SIZE + 1)) | (x_local == 0) | (x_local == (BLOCK_SIZE + 1)));

    int nb_count = 0;

    // indexing like this is only legal in the actually calculated cells
    if (is_compute_cell) {
        nb_count +=
            group_board[(y_local - 1) * LOCAL_MEM_BLOCK_SIZE + (x_local - 1)] + group_board[(y_local - 1) * LOCAL_MEM_BLOCK_SIZE + (x_local)] + group_board[(y_local - 1) * LOCAL_MEM_BLOCK_SIZE + (x_local + 1)] +
            group_board[(y_local    ) * LOCAL_MEM_BLOCK_SIZE + (x_local - 1)] +                                                                 group_board[(y_local    ) * LOCAL_MEM_BLOCK_SIZE + (x_local + 1)] +
            group_board[(y_local + 1) * LOCAL_MEM_BLOCK_SIZE + (x_local - 1)] + group_board[(y_local + 1) * LOCAL_MEM_BLOCK_SIZE + (x_local)] + group_board[(y_local + 1) * LOCAL_MEM_BLOCK_SIZE + (x_local + 1)]
        ;
    }

    bool would_stay_alive = (2 <= nb_count) & (nb_count <= 3);
    bool would_gain_live = nb_count == 3;

    bool is_alive_now = was_alive ? would_stay_alive : would_gain_live;

    // "true" x and y position on the board (not bloated by larger kernel workgroup size for caching, wrapped around)
    int y = fixed_modulo(y_group * BLOCK_SIZE + (y_local - 1), y_width);
    int x = fixed_modulo(x_group * BLOCK_SIZE + (x_local - 1), x_width);
    // if this is a cell at the border its only real purpose was to write the local memory. I left the other stuff there because it will still realistically be executed like this
    // barrier(CLK_GLOBAL_MEM_FENCE);
    if (is_compute_cell) {
        board[y * y_width + x] = is_alive_now;
    }
}

int main(int argc, char* argv[]) {
    uint board_height = std::stoi(argv[1]);
    uint board_width = std::stoi(argv[2]);
    int generations = std::stoi(argv[3]);
    bool display = std::stoi(argv[4]);

    auto game_field = std::make_unique<bool[]>(board_height * board_width);
    game_field[1 + board_width * 2] = true;
    game_field[2 + board_width * 3] = true;
    game_field[3 + board_width * 1] = true;
    game_field[3 + board_width * 2] = true;
    game_field[3 + board_width * 3] = true;

    TimedScope optimized_gol_timer("OMP VER GoL timer");

    if (display) {        
        display_board_state(game_field.get(), board_width, board_height);
    }

    int group_rows = board_height / BLOCK_SIZE;
    int group_cols = board_width / BLOCK_SIZE;
    
    for (int iteration = 0; iteration < generations; iteration ++) {
        #pragma omp parallel for collapse(2)
        for (int group_row = 0; group_row < group_rows; group_row++) {
            for (int group_col = 0; group_col < group_cols; group_col++) {
                bool local_field[LOCAL_MEM_BLOCK_SIZE * LOCAL_MEM_BLOCK_SIZE];
                for (int row = 0; row < LOCAL_MEM_BLOCK_SIZE; row++) {
                    for (int column = 0; column < LOCAL_MEM_BLOCK_SIZE; column++) {
                        game_of_life_split_util_local_mem(board_width, board_height, game_field.get(), local_field, row, column, group_row, group_col);
                    }
                }

                for (int row = 0; row < LOCAL_MEM_BLOCK_SIZE; row++) {
                    for (int column = 0; column < LOCAL_MEM_BLOCK_SIZE; column++) {
                        game_of_life_split_util_global_mem(board_width, board_height, game_field.get(), local_field, row, column, group_row, group_col);
                    }
                }
            }
        }

        if (display) {
            printf(" --- gen %d ------------", iteration);
            display_board_state(game_field.get(), board_width, board_height);
        }
    }
}

void display_board_state(bool * board, uint board_width, uint board_height)
{
    fprintf(stdout, "\n");
    for (uint row = 0; row < board_height; row++) {
        for (uint column = 0; column < board_width; column++) {
            auto repr = board[row * board_width + column] ? " *" : " -";
            fprintf(stdout, "%s", repr);
        }
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n");
}