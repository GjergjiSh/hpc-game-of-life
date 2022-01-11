#define BLOCK_SIZE 8

__kernel void game_of_life_split(
    __global bool* b,
    __local bool* a_block
)
{
    // b[0] = true;
    // TODO actually replace with a working kernel instead of the matrixmul-example
    // int y = get_global_id(0);
    // int x = get_global_id(1);
    // int ylocal = get_local_id(0);
    // int xlocal = get_local_id(1);

    // int yblock = get_group_id(0);
    // int xblock = get_group_id(1);

    // float tmp = 0.0;
    // for(int kblock = 0; kblock < N_BLOCKS; kblock++)
    // {
    //     // Cache in local memory a and b values of current block
    //     // Cache for whole work group
    //     // Each work item reads exactly 1 value of a and b
    //     // To differentiate take local id

    //     // Block for a: starts at:
    //     // rows/y: same as c -> yblock
    //     // cols/x: same as k -> kblock
    //     // Starting adress of block for
    //     int abase = yblock * BLOCK_SIZE * N + kblock * BLOCK_SIZE;
    //     // Add to that local position in global memory:
    //     int local_global = ylocal * N + xlocal;
    //     // Store to local position in local memory
    //     int local_local = ylocal * BLOCK_SIZE + xlocal;
    //     a_block[local_local] = a[abase + local_global];
    //     // Block for b: starts at:
    //     // rows/y: same as k -> kblock
    //     // cols/x: same as c -> xblock
    //     // Starting adress of block for
    //     int bbase = kblock * BLOCK_SIZE * N + xblock * BLOCK_SIZE;
    //     // Add to that local position (same as before):
    //     b_block[local_local] = b[bbase + local_global];

    //     // Wait that all work items are done caching
    //     barrier(CLK_LOCAL_MEM_FENCE);
    //     #pragma unroll
    //     for (int k = 0; k < BLOCK_SIZE; k++)
    //     {
    //         tmp += a_block[ylocal * BLOCK_SIZE + k] * b_block[k * BLOCK_SIZE + xlocal];
    //     } 
    //     // Wait again for all work items before we overwrite cache in next iteration
    //     barrier(CLK_LOCAL_MEM_FENCE);
    // }
    // c[y * N + x] += tmp;
}