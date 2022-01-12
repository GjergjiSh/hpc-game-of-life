#define BLOCK_SIZE 8

/// fixes modulo for the purposes of this file (maximally negative the modulo range). Also may whoever decided Cs modulo behavior was right like it is suffer...
int fixed_modulo(int val, int mod) {
    return (val + mod) % mod;
}

kernel void game_of_life_split(
    global bool* board,
    local bool* group_board
)
{
    int y = get_global_id(0);
    int x = get_global_id(1);

    int y_width = get_global_size(0);
    int x_width = get_global_size(1);
    int tot_size = y_width * x_width;

    // int y_group = get_group_id(0);
    // int x_group = get_group_id(1);

    int y_local = get_local_id(0);
    int x_local = get_local_id(1);

    // write the value to the local cache
    group_board[(1 + y_local) * BLOCK_SIZE + (x_local + 1)] = board[y * y_width + x];
    // also the borders
    if (y_local == 0) group_board[x_local + 1] = board[fixed_modulo((y - 1) * y_width + x, tot_size)]; // lower border
    else if (y_local == (BLOCK_SIZE - 1)) group_board[(1 + BLOCK_SIZE) * BLOCK_SIZE + (x_local + 1)] = board[fixed_modulo((y + 1) * y_width + x, tot_size)]; // upper border
    else if (x_local == 0) group_board[(1 + y_local) * BLOCK_SIZE] = board[fixed_modulo(y * y_width + x - 1, tot_size)]; // left border except corners
    else if (x_local == (BLOCK_SIZE - 1)) group_board[(1 + y_local) * BLOCK_SIZE + BLOCK_SIZE + 1] = board[fixed_modulo(y * y_width + x + 1, tot_size)]; // right border except corners

    barrier(CLK_LOCAL_MEM_FENCE);

    int nb_count =
    group_board[(y_local    ) * BLOCK_SIZE + (x_local)] + group_board[(y_local    ) * BLOCK_SIZE + (x_local + 1)] + group_board[(y_local    ) * BLOCK_SIZE + (x_local + 2)] +
    group_board[(y_local + 1) * BLOCK_SIZE + (x_local)] +                                                           group_board[(y_local + 1) * BLOCK_SIZE + (x_local + 2)] +
    group_board[(y_local + 2) * BLOCK_SIZE + (x_local)] + group_board[(y_local + 2) * BLOCK_SIZE + (x_local + 1)] + group_board[(y_local + 2) * BLOCK_SIZE + (x_local + 2)];

    bool was_alive = group_board[(y_local + 1) * BLOCK_SIZE + (x_local + 1)];
    bool would_stay_alive = 2 <= nb_count && nb_count <= 3;
    bool would_gain_live = nb_count == 3;

    bool is_alive_now = was_alive ? would_stay_alive : would_gain_live;

    barrier(CLK_GLOBAL_MEM_FENCE);
    board[y * y_width + x] = is_alive_now;
}