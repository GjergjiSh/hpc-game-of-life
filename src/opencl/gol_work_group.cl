// #include <opencl-c-base.h>
#define BLOCK_SIZE 8
#define LOCAL_MEM_BLOCK_SIZE (BLOCK_SIZE + 2)

/// fixes modulo for the purposes of this file (maximally negative the modulo range). Also may whoever decided Cs modulo behavior was right like it is suffer...
int fixed_modulo(int val, int mod) {
    return (val + mod) % mod;
}

kernel void game_of_life_split(
    int x_width,
    int y_width,
    global bool* board,
    local bool* group_board // has size (BLOCK_SIZE + 2)²
)
{
    int y_local = get_local_id(0);
    int x_local = get_local_id(1);

    int y_group = get_group_id(0);
    int x_group = get_group_id(1);

    // "true" x and y position on the board (not bloated by larger kernel workgroup size for caching, wrapped around)
    int y = fixed_modulo(y_group * BLOCK_SIZE + (y_local - 1), y_width);
    int x = fixed_modulo(x_group * BLOCK_SIZE + (x_local - 1), x_width);

    bool was_alive = board[y * x_width + x];
    group_board[y_local * LOCAL_MEM_BLOCK_SIZE + x_local] = was_alive;

    // if this is a cell at the border its only real purpose was to write the local memory. I left the other stuff there because it will still realistically be executed like this
    bool is_compute_cell = !((y_local == 0) | (y_local == (BLOCK_SIZE + 1)) | (x_local == 0) | (x_local == (BLOCK_SIZE + 1)));

    int nb_count = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
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

    // printf("(%2d, %2d) : (%d, %d)", x, y, x_local, y_local);

    // if (is_compute_cell) {
    //     printf("\t nb: %d, %d -> %d", nb_count, was_alive, is_alive_now);
    // }

    // printf("\n");

    // if this is a cell at the border its only real purpose was to write the local memory. I left the other stuff there because it will still realistically be executed like this
    barrier(CLK_GLOBAL_MEM_FENCE);
    if (is_compute_cell) {
        board[y * y_width + x] = is_alive_now;
    }
}