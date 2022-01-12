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
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#endif

// path relative to the makefile where the programm gets executed from 
#define CL_PROGRAM_FILE "src/optimized/gol_work_group.cl"
// has to be the name of the choosen kernel inside the cl file 
#define KERNEL_NAME "game_of_life_split"
#define MAX_SELECTIONS 10
#define MAX_NAME_LENGTH 150


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

    ClBufferRepr(cl_context context, cl_mem_flags flags, size_t size) {
        mem = clCreateBuffer(context, CL_MEM_READ_ONLY, size, NULL, NULL);
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
    
    void write_mem(cl_command_queue queue, size_t size, void* src) {
        // FIXME: Potential performance improvement with non-blocking write
        int err = clEnqueueWriteBuffer(queue, mem, CL_TRUE, 0, size, src, 0, NULL, NULL);
        if (err != CL_SUCCESS)
        {
            fprintf(stderr, "Failed to write to device array\n");
            exit(-1);
        }
    }

    ~ClBufferRepr() {
        clReleaseMemObject(mem);
    }
};

#define BLOCK_SIZE 8

void execute(ClContextEtc& cl_ctx_etc, bool* start_board, uint board_height, uint board_width) {
    int err = 0;

    uint board_size = board_height * board_width;
    uint board_mem_size = board_size * sizeof(bool);

    // Daten auf dem Host vorbereiten
    auto h_board = std::make_unique<bool[]>(board_mem_size);

    memcpy(h_board.get(), start_board, board_mem_size);

    // Create array in device memory
    ClBufferRepr d_board(cl_ctx_etc.context, CL_MEM_READ_WRITE, board_mem_size);

    // Copy data to device
    d_board.write_mem(cl_ctx_etc.queue, board_mem_size, h_board.get());

    // FIXME: potential issue caused by my removal of clFinish. I removed a few more above, but this here is a step before clEnqueueNDRangeKernel
    // Wait for transfers to finish
    // clFinish(cl_ctx_etc.queue);
    
    // Set the arguments to our compute kernel
    cl_int board_width_int = (cl_int) board_width;
    cl_int board_height_int = (cl_int) board_height;
    err |= clSetKernelArg(cl_ctx_etc.kernel, 0, sizeof(cl_int), &board_width_int);
    err |= clSetKernelArg(cl_ctx_etc.kernel, 1, sizeof(cl_int), &board_height_int);
    // source memory
    err |= clSetKernelArg(cl_ctx_etc.kernel, 2, sizeof(cl_mem), &d_board);
    // workgroup copy for caching of reads (2 larger in each dimension for reads to the border)
    err |= clSetKernelArg(cl_ctx_etc.kernel, 3, sizeof(bool) *  (2 + BLOCK_SIZE) * (2 + BLOCK_SIZE), NULL);
    if (err != CL_SUCCESS)
    {
        fprintf(stderr, "Failed to set kernel arguments!\n");
        exit(-1);
    }

    // dont want conditionals in every workitem, cant figure out another way
    // ensure work can get cleanly seperated into the other workitems
    if (board_width % BLOCK_SIZE != 0 || board_height % BLOCK_SIZE != 0) throw std::exception();
    // we add an additional dimension for memory requests, so we have at most 1 array request to global storage per kernel. For that to work we need enough work_items in that throwaway dimension
    // ALTERNATIVE: calculate the size of that dimension so that with BLOCK_SIZE² * (mem_dims - 1) we have enough tasks
    if (BLOCK_SIZE * BLOCK_SIZE < (4 * BLOCK_SIZE + 4)) throw std::exception();

    // Execute the kernel
    // local cache needs border values too, we will just have the border cases of this kernel not write their values to global memory. This means we need more local kernels
    size_t local_size[] = { BLOCK_SIZE + 2, BLOCK_SIZE + 2 };
    // the global kernels expand accordingly
    // the terms generated by the above expansion---------\/--------------------------------------------\/--------
    // TODO: Issue here with the sizes
    size_t global_size[] = { board_height + (board_height / BLOCK_SIZE) * 2 , board_width + (board_width / BLOCK_SIZE) * 2 }; //  // + (board_width / BLOCK_SIZE + 1) * 2 
    err = clEnqueueNDRangeKernel(cl_ctx_etc.queue, cl_ctx_etc.kernel, 2, NULL, global_size, local_size, 0, NULL, NULL);
    if (err)
    {
        fprintf(stderr, "Failed to execute kernel! %d\n", err);
        exit(-1);
    }

    // FIXME: potential issue caused by my removal of clFinish.
    // Wait for the commands to get serviced before reading back results
    // clFinish(cl_ctx_etc.queue);
    
    // Copy Result Data Back
    err = clEnqueueReadBuffer(cl_ctx_etc.queue, d_board.mem, CL_TRUE, 0, board_mem_size, h_board.get(), 0, NULL, NULL);

    if (err != CL_SUCCESS)
    {
        fprintf(stderr, "Failed to read from device array\n");
        exit(-1);
    }
    // Wait for commands to finish (before dropping the kernel, program, etc)
    clFinish(cl_ctx_etc.queue);
    fprintf(stdout, "%d\n", h_board[0]);
}

void test() {
    auto program_text = read_program_text();
    auto device = select_device();

    ClContextEtc cl_ctx_etc(device, std::move(program_text), KERNEL_NAME);
    uint board_height = 16;
    uint board_width = 16;

    auto start_field = std::make_unique<bool[]>(board_height * board_width);
    start_field[1 + board_width * 2] = true;
    start_field[2 + board_width * 3] = true;
    start_field[3 + board_width * 1] = true;
    start_field[3 + board_width * 2] = true;
    start_field[3 + board_width * 3] = true;

    execute(cl_ctx_etc, start_field.get(), 16, 16);
}