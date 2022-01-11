#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <memory>
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#define CL_TARGET_OPENCL_VERSION 220
#include <CL/cl.h>
#endif

// path relative to the makefile where the programm gets executed from 
#define CL_PROGRAM_FILE "src/optimized/gol_work_group.cl"
// has to be the name of the choosen kernel inside the cl file 
#define KERNEL_NAME "game_of_life_split"
#define MAX_SELECTIONS 10
#define MAX_NAME_LENGTH 150


char* read_program_text() {
    // Load program from file
    FILE *program_file = fopen(CL_PROGRAM_FILE, "r");
    if(program_file == NULL) {
        fprintf(stderr, "Failed to open OpenCL program file\n");
        exit(-1);
    }
    fseek(program_file, 0, SEEK_END);
    size_t program_size = ftell(program_file);
    rewind(program_file);
    char *program_text = (char *) malloc((program_size + 1) * sizeof(char));
    program_text[program_size] = '\0';
    fread(program_text, sizeof(char), program_size, program_file);
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

    char name[MAX_NAME_LENGTH];
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

#define N 1024
#define BLOCK_SIZE 16
#define N_BLOCKS (N / BLOCK_SIZE)
#define MATRIX_SIZE (N * N)
#define MATRIX_MEM  (N * N * sizeof(float))

struct ClContextEtc {
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;

    ClContextEtc(cl_device_id device, char *program_text, char *kernel_name) {
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

        int err;
        program = clCreateProgramWithSource(context, 1, (const char **) &program_text, NULL, &err);
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
        mem = clCreateBuffer(context, CL_MEM_READ_ONLY, MATRIX_MEM, NULL, NULL);
        if(!mem)
        {
            fprintf(stderr, "Failed to allocate memory on device\n");
            exit(-1);
        }
    }

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

int execute(cl_device_id device, char *program_text, char *kernel_name) {
    ClContextEtc cl_ctx_etc(device, program_text, kernel_name);
    
    int err = 0;

    // Daten auf dem Host vorbereiten
    float *h_a = new float[MATRIX_MEM];
    float *h_b = new float[MATRIX_MEM];
    float *h_c = new float[MATRIX_MEM];
    for(size_t i=0; i<MATRIX_SIZE;i++) 
    {
        h_a[i] = 2.0;
        h_b[i] = 4.0;
        h_c[i] = 0.0;
    }

    // TODO actually replace with a working kernel instead of the modified matrixmul-example

    // Create array in device memory
    ClBufferRepr d_a(cl_ctx_etc.context, CL_MEM_READ_ONLY, MATRIX_MEM);
    ClBufferRepr d_b(cl_ctx_etc.context, CL_MEM_READ_ONLY, MATRIX_MEM);
    ClBufferRepr d_c(cl_ctx_etc.context, CL_MEM_READ_WRITE, MATRIX_MEM);

    // Copy data to device
    d_a.write_mem(cl_ctx_etc.queue, MATRIX_MEM, h_a);
    d_b.write_mem(cl_ctx_etc.queue, MATRIX_MEM, h_b);
    d_c.write_mem(cl_ctx_etc.queue, MATRIX_MEM, h_c);

    // FIXME: potential issue caused by my removal of clFinish. I removed a few more above, but this here is a step before clEnqueueNDRangeKernel
    // Wait for transfers to finish
    // clFinish(commands);
    
    // Set the arguments to our compute kernel
    err |= clSetKernelArg(cl_ctx_etc.kernel, 0, sizeof(cl_mem), &d_a);
    err |= clSetKernelArg(cl_ctx_etc.kernel, 1, sizeof(cl_mem), &d_b);
    err |= clSetKernelArg(cl_ctx_etc.kernel, 2, sizeof(cl_mem), &d_c);
    err |= clSetKernelArg(cl_ctx_etc.kernel, 3, sizeof(float) * BLOCK_SIZE * BLOCK_SIZE, NULL);
    err |= clSetKernelArg(cl_ctx_etc.kernel, 4, sizeof(float) * BLOCK_SIZE * BLOCK_SIZE, NULL);
    if (err != CL_SUCCESS)
    {
        fprintf(stderr, "Failed to set kernel arguments!\n");
        exit(-1);
    }

    // Execute the kernel
    size_t global_size[] = {N, N};
    size_t local_size[] = {BLOCK_SIZE, BLOCK_SIZE};
    err = clEnqueueNDRangeKernel(cl_ctx_etc.queue, cl_ctx_etc.kernel, 2, NULL, global_size, local_size, 0, NULL, NULL);
    if (err)
    {
        fprintf(stderr, "Failed to execute kernel! %d\n", err);
        exit(-1);
    }

    // FIXME: potential issue caused by my removal of clFinish.
    // Wait for the commands to get serviced before reading back results
    // clFinish(commands);
    
    // Copy Result Data Back
    err = clEnqueueReadBuffer(cl_ctx_etc.queue, d_c.mem, CL_TRUE, 0, MATRIX_MEM, h_c, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        fprintf(stderr, "Failed to read from device array\n");
        exit(-1);
    }
    // Wait for transfer to finish
    clFinish(cl_ctx_etc.queue);

    // Check Results
    int errors = 0;
    for(int i=0; i<MATRIX_SIZE;i++) {
        if(fabs(h_c[i] - N * 2.0 * 4.0) > 0.0001) {
            errors += 1;
            printf("%d %f %f %f\n", i, h_c[i], h_a[i], h_b[i]);
        }
    }

    delete[] h_a;
    delete[] h_b;
    delete[] h_c;
    return errors;
}

void test() {
    char* program_text = read_program_text();
    auto device = select_device();

    execute(device, program_text, KERNEL_NAME);
}