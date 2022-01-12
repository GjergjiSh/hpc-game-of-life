#include <cstdio>
#define BLOCK_SIZE 2

/// fixes modulo for the purposes of this file (maximally negative the modulo range). Also may whoever decided Cs modulo behavior was right like it is suffer...
int fixed_modulo(int val, int mod) {
    return (val + mod) % mod;
}

int main() {
    const int tot_size = 4 * 4;
    int y_width = 4;
    int x_width = 4;

    bool board[tot_size] {
        true, false, false, true,
        true, true, false, false,
        false, true, true, false,
        false, false, true, true,
    };

    bool group_board[tot_size] = {};

    for (auto y_local = 2; y_local < 4; y_local ++) {
        for (auto x_local = 0; x_local < 2; x_local ++) {
            int x = x_local + 2;
            int y = y_local + 0;

            fprintf(stdout, "write_local, pos %d %d", x, y);

            int lower_bord_tot_pos = fixed_modulo((y - 1) * y_width + x, tot_size);

            if (y_local == 0 && x_local != 0) group_board[x_local + 1] = board[fixed_modulo((y - 1) * y_width + x, tot_size)]; // lower border
            if (y_local == (BLOCK_SIZE - 1)) group_board[(1 + BLOCK_SIZE) * BLOCK_SIZE + (x_local + 1)] = board[fixed_modulo((y + 1) * y_width + x, tot_size)]; // upper border
            if (x_local == 0) group_board[(1 + y_local) * BLOCK_SIZE] = board[fixed_modulo(y * y_width + x - 1, tot_size)]; // left border except corners
            if (x_local == (BLOCK_SIZE - 1)) group_board[(1 + y_local) * BLOCK_SIZE + BLOCK_SIZE + 1] = board[fixed_modulo(y * y_width + x + 1, tot_size)]; // right border except corners

        }
    }

    for (auto y = 0; y < 4; y ++) {
        for (auto x = 0; x < 4; x ++) {
            fprintf(stdout, "%d\t", group_board[y * BLOCK_SIZE + x]);
        }
        fprintf(stdout, "\n");
    }
}