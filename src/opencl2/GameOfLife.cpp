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

// path relative to the makefile where the programm gets executed from 
#define CL_PROGRAM_FILE "src/opencl2/gol_work_group.cl"
// has to be the name of the choosen kernel inside the cl file 
#define KERNEL_NAME "game_of_life_split"
#define MAX_SELECTIONS 10
#define MAX_NAME_LENGTH 150

#define BLOCK_SIZE 8
#define MEM_BLOCK_SIZE BLOCK_SIZE + 2

void display_board_state(bool * board, uint board_width, uint board_height);

std::unique_ptr<char[]> read_program_text() {
    // Load program from file
    FILE *program_file = fopen(CL_PROGRAM_FILE, "r");
    if(program_file == NULL) {
        fprintf(stderr, "Failed to open OpenCL program file\n");
        exit(-1);
    }
    fseek(program_file, 0, SEEK_END);
    size_t program_size = ftell(program_file);
    rewind(program_file);
    auto program_text = std::make_unique<char[]>(program_size + 1);
    program_text[program_size] = '\0';
    fread(program_text.get(), sizeof(char), program_size, program_file);
    fclose(program_file);
    return program_text;
}

cl_device_id select_device() {
    // First get available platforms
    cl_platform_id *platforms;
    cl_uint n_platforms;
    clGetPlatformIDs(MAX_SELECTIONS, NULL, &n_platforms);
    platforms = (cl_platform_id *) malloc(n_platforms * sizeof(cl_platform_id));
    clGetPlatformIDs(n_platforms, platforms, NULL);

    if(n_platforms == 0) {
        fprintf(stderr, "No platforms found. Exiting.");
        exit(-1);
    }

    // char name[MAX_NAME_LENGTH];
    // printf("Platforms:\n");
    // for(uint i=0;i<n_platforms;i++) 
    // {
    //     clGetPlatformInfo(platforms[i], CL_PLATFORM_NAME, sizeof(name), name, NULL);
    //     printf("[%d]: %s\n", i, name);
    // }
    // printf("Choose platform (Default 0): ");
    uint choice = 0;
    // scanf("%d", &choice);
    if(choice < 0 || choice >= n_platforms) {
        printf("Invalid choice, using platform 0");
        choice = 0;
    }
    cl_platform_id platform;
    memcpy(&platform, &platforms[choice], sizeof(cl_platform_id));
    free(platforms);

    // Get Devices for chosen platform
    cl_device_id *devices;
    cl_uint n_devices;
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, MAX_SELECTIONS, NULL, &n_devices);
    devices = (cl_device_id *) malloc(n_devices * sizeof(cl_device_id));
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, n_devices, devices, NULL);
    if(n_devices == 0) {
        fprintf(stderr, "No devices found for platform. Exiting.\n");
        exit(-1);
    }


    // printf("Devices:\n");
    // for(uint i=0;i<n_devices;i++) {
    //     clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(name), name, NULL);
    //     printf("[%d]: %s\n", i, name);
    // }
    // printf("Choose device (Default 0): ");
    choice = 0;
    // scanf("%d", &choice);
    if(choice < 0 || choice >= n_devices) {
        printf("Invalid choice, using device 0");
        choice = 0;
    }
    cl_device_id device;
    memcpy(&device, &devices[choice], sizeof(cl_device_id));
    free(devices);
    return device;
}

/// Initializes and holds the OpenCL context, program and kernel, as well as a command queue
struct ClContextEtc {
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;

    ClContextEtc(cl_device_id device, std::unique_ptr<char[]> program_text, char *kernel_name) {
        context = clCreateContext(0, 1, &device, NULL, NULL, NULL);
        if (!context)
        {
            fprintf(stderr, "Failed to create a compute context.\n");
            exit(-1);
        }

        // Create a command queue
        queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, NULL);
        if (!queue)
        {
            fprintf(stderr, "Failed to create a command queue.\n");
            exit(-1);
        }

        char* program_text1 = program_text.get(); 

        int err;
        program = clCreateProgramWithSource(context, 1, (const char **) &program_text1, NULL, &err);
        if(err < 0)
        {
            fprintf(stderr, "Failed to create program.\n");
            exit(-1);
        }

        err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
        if (err != CL_SUCCESS)
        {
            size_t len;
            char buffer[2048];

            fprintf(stderr, "Failed to build program executable!\n");
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
            fprintf(stderr, "%s\n", buffer);
            exit(-1);
        }

        // Create Kernels
        kernel = clCreateKernel(program, kernel_name, &err);
        if (!kernel || err != CL_SUCCESS)
        {
            fprintf(stderr, "Failed to create compute kernel!\n");
            exit(-1);
        }
    }

    ~ClContextEtc() {
        clReleaseProgram(program);
        clReleaseKernel(kernel);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
    }
};

struct ClBufferRepr {
    cl_mem mem;
    size_t size;

    ClBufferRepr(cl_context context, cl_mem_flags flags, size_t size) {
        mem = clCreateBuffer(context, CL_MEM_READ_ONLY, size, NULL, NULL);
        this->size = size;
        if(!mem)
        {
            fprintf(stderr, "Failed to allocate memory on device\n");
            exit(-1);
        }
    }

    // ClBufferRepr(ClBufferRepr &) = default;
    // ClBufferRepr& operator=(ClBufferRepr &) = default;

    // ClBufferRepr(ClBufferRepr const&) = delete;
    // ClBufferRepr& operator=(ClBufferRepr const&) = delete;
    
    void write_mem(cl_command_queue queue, void* src) {
        // FIXME: Potential performance improvement with non-blocking write
        int err = clEnqueueWriteBuffer(queue, mem, CL_TRUE, 0, size, src, 0, NULL, NULL);
        if (err != CL_SUCCESS)
        {
            fprintf(stderr, "Failed to write to device array\n");
            exit(-1);
        }
    }

    void get_vals(cl_command_queue queue, void* dst) {
        int err = clEnqueueReadBuffer(queue, mem, CL_TRUE, 0, size, dst, 0, NULL, NULL);
        if (err != CL_SUCCESS)
        {
            fprintf(stderr, "Failed to read from device array\n");
            exit(-1);
        }
    }

    ~ClBufferRepr() {
        clReleaseMemObject(mem);
    }
};

void execute(ClContextEtc& cl_ctx_etc, bool* start_board, uint board_height, uint board_width, uint num_iterations, bool display) {
    int err = 0;

    uint board_size = board_height * board_width;
    uint board_mem_size = board_size * sizeof(bool);

    // Daten auf dem Host vorbereiten
    auto h_board = std::make_unique<bool[]>(board_mem_size);

    memcpy(h_board.get(), start_board, board_mem_size);

    // Create array in device memory
    ClBufferRepr d_board1(cl_ctx_etc.context, CL_MEM_READ_WRITE, board_mem_size);
    ClBufferRepr d_board2(cl_ctx_etc.context, CL_MEM_READ_WRITE, board_mem_size);

    // Copy data to device
    d_board1.write_mem(cl_ctx_etc.queue, h_board.get());
    
    // Set the arguments to our compute kernel
    cl_int board_width_int = (cl_int) board_width;
    cl_int board_height_int = (cl_int) board_height;
    err |= clSetKernelArg(cl_ctx_etc.kernel, 0, sizeof(cl_int), &board_width_int);
    err |= clSetKernelArg(cl_ctx_etc.kernel, 1, sizeof(cl_int), &board_height_int);
    // source memory
    err |= clSetKernelArg(cl_ctx_etc.kernel, 2, sizeof(cl_mem), &d_board1);
    err |= clSetKernelArg(cl_ctx_etc.kernel, 3, sizeof(cl_mem), &d_board2);
    // workgroup copy for caching of reads (2 larger in each dimension for reads to the border)
    err |= clSetKernelArg(cl_ctx_etc.kernel, 4, sizeof(bool) *  MEM_BLOCK_SIZE * MEM_BLOCK_SIZE, NULL);
    if (err != CL_SUCCESS)
    {
        fprintf(stderr, "Failed to set kernel arguments!\n");
        exit(-1);
    }

    // dont want conditionals in every workitem, cant figure out another way
    // ensure work can get cleanly seperated into the other workitems
    if (board_width % BLOCK_SIZE != 0 || board_height % BLOCK_SIZE != 0) throw std::exception();
    if (BLOCK_SIZE * BLOCK_SIZE < (4 * BLOCK_SIZE + 4)) throw std::exception();

    // Execute the kernel
    // local cache needs border values too, we will just have the border cases of this kernel not write their values to global memory. This means we need more local kernels
    size_t local_size[] = { MEM_BLOCK_SIZE,  MEM_BLOCK_SIZE};
    // the global kernels expand accordingly
    // the terms generated by the above expansion---------\/--------------------------------------------\/--------
    size_t global_size[] = { board_height + (board_height / BLOCK_SIZE) * 2 , board_width + (board_width / BLOCK_SIZE) * 2 }; //  // + (board_width / BLOCK_SIZE + 1) * 2 


    if (display) {
        display_board_state(h_board.get(), board_width, board_height);
    }

    TimedScope optimized_gol_timer("CL2 GoL timer");

    for (uint iteration = 0; iteration < num_iterations; iteration ++) {
        int switch_kernels = iteration % 2;
        err |= clSetKernelArg(cl_ctx_etc.kernel, 2 + switch_kernels, sizeof(cl_mem), &d_board1);
        err |= clSetKernelArg(cl_ctx_etc.kernel, 3 - switch_kernels, sizeof(cl_mem), &d_board2);
        if (err != CL_SUCCESS)
        {
            fprintf(stderr, "Failed to set kernel arguments!\n");
            exit(-1);
        }
        err = clEnqueueNDRangeKernel(cl_ctx_etc.queue, cl_ctx_etc.kernel, 2, NULL, global_size, local_size, 0, NULL, NULL);
        if (err)
        {
            fprintf(stderr, "Failed to execute kernel! %d\n", err);
            exit(-1);
        }

        if (display) {        
            //Copy Result Data Back
            if (switch_kernels) d_board1.get_vals(cl_ctx_etc.queue, h_board.get());
            else d_board2.get_vals(cl_ctx_etc.queue, h_board.get());
            // Wait for the commands to get serviced before reading back results
            clFinish(cl_ctx_etc.queue);
            printf(" --- gen %d ------------", iteration);
            display_board_state(h_board.get(), board_width, board_height);
        }
        
    }
    // Wait for commands to finish (before dropping the kernel, program, etc)
    clFinish(cl_ctx_etc.queue);
}


int main(int argc, char* argv[]) {
    uint board_height = std::stoi(argv[1]);
    uint board_width = std::stoi(argv[2]);
    int generations = std::stoi(argv[3]);
    bool display = std::stoi(argv[4]);
    auto program_text = read_program_text();
    auto device = select_device();

    ClContextEtc cl_ctx_etc(device, std::move(program_text), KERNEL_NAME);

    auto start_field = std::make_unique<bool[]>(board_height * board_width);
    start_field[1 + board_width * 2] = true;
    start_field[2 + board_width * 3] = true;
    start_field[3 + board_width * 1] = true;
    start_field[3 + board_width * 2] = true;
    start_field[3 + board_width * 3] = true;

    execute(cl_ctx_etc, start_field.get(), board_height, board_width, generations, display);
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