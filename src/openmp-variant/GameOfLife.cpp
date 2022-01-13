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

    auto start_field = std::make_unique<bool[]>(board_height * board_width);
    start_field[1 + board_width * 2] = true;
    start_field[2 + board_width * 3] = true;
    start_field[3 + board_width * 1] = true;
    start_field[3 + board_width * 2] = true;
    start_field[3 + board_width * 3] = true;

    TimedScope optimized_gol_timer("OMP VER GoL timer");

    if (display) {        
        display_board_state(start_field.get(), board_width, board_height);
    }

    for (uint iteration = 0; iteration < generations; iteration ++) {
        #pragma omp for collapse(2)
        for (uint group_row = 0; group_row < 2; group_row++) {
            for (uint group_col = 0; group_col < 2; group_col++) {
                bool test_field[LOCAL_MEM_BLOCK_SIZE * LOCAL_MEM_BLOCK_SIZE];
                for (uint row = 0; row < LOCAL_MEM_BLOCK_SIZE; row++) {
                    for (uint column = 0; column < LOCAL_MEM_BLOCK_SIZE; column++) {
                        game_of_life_split_util_local_mem(board_width, board_height, start_field.get(), test_field, row, column, group_row, group_col);
                    }
                }

                for (uint row = 0; row < LOCAL_MEM_BLOCK_SIZE; row++) {
                    for (uint column = 0; column < LOCAL_MEM_BLOCK_SIZE; column++) {
                        game_of_life_split_util_global_mem(board_width, board_height, start_field.get(), test_field, row, column, group_row, group_col);
                    }
                }
            }
        }

        if (display) {        
            display_board_state(start_field.get(), board_width, board_height);
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