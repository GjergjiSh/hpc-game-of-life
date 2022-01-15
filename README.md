# HPC Game of Life

## Usage

* Build by running `make all` or `make bench`. (Make bench runs versions of GoL automatically for comparison)
* Block-Sizes for OpenCL need to be manually adjusted in the source-code, in the kernel with fileextension .cl and the GameOfLife.cpp file. In each file there is a #define BLOCK_SIZE. They exist in each implementation seperately, so thats 4 instances of BLOCK_SIZE that have to be changed to gain comparability.
* `make docs` builds the report.
