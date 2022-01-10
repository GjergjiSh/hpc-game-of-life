INC_PATH = -I./include/

# Includes all cpp files. You cen debug these kind of things with rules with @echo
SRC:=$(wildcard src/**/*.cpp)

# We generate the object files aside the cpp file, with .opt.o/.unopt.o file endings with/without compiler optimizations
UNOPT_OBJ:=$(patsubst %.cpp, %.unopt.o, $(SRC))
OPT_OBJ:=$(patsubst %.cpp, %.opt.o, $(SRC))

# The cpp-compiler with the default flags we always provide
BASE_CC = g++ -std=c++17 $(INC_PATH) -Wno-write-strings -Wall -DNDEBUG

UNOPT_CC = $(BASE_CC)
OPT_CC = $(BASE_CC) -fopenmp -O3

# The rules to generate the optimized and unoptimized object files
$(UNOPT_OBJ): %.unopt.o: %.cpp
	$(UNOPT_CC) -c $< -o $@

$(OPT_OBJ): %.opt.o: %.cpp
	$(OPT_CC) -c $< -o $@

BASE_LD = g++ -DNDEBUG -Wl,--no-as-needed
UNOPT_LD = $(BASE_LD)
OPT_LD = $(BASE_LD) -O3

# The dependencies of the manually optimized or not GameOfLife
MO_GoL_DEPS = common/Timing optimized/GameOfLife optimized/Main
MU_GoL_DEPS = common/Timing unoptimized/GameOfLife unoptimized/Main

# TODO DOC
MO_CO_OBJ = $(patsubst %, src/%.opt.o, $(MO_GoL_DEPS))
GameOfLife.mo.co.out: $(MO_CO_OBJ)
	$(OPT_LD) -o GameOfLife.mo.co.out $(MO_CO_OBJ)

MU_CO_OBJ = $(patsubst %, src/%.opt.o, $(MU_GoL_DEPS))
GameOfLife.mu.co.out: $(MU_CO_OBJ)
	$(OPT_LD) -o GameOfLife.mu.co.out $(MU_CO_OBJ)

MO_CU_OBJ = $(patsubst %, src/%.unopt.o, $(MO_GoL_DEPS))
GameOfLife.mo.cu.out: $(MO_CU_OBJ)
	$(UNOPT_LD) -o GameOfLife.mo.cu.out $(MO_CU_OBJ)

MU_CU_OBJ = $(patsubst %, src/%.unopt.o, $(MU_GoL_DEPS))
GameOfLife.mu.cu.out: $(MU_CU_OBJ)
	$(UNOPT_LD) -o GameOfLife.mu.cu.out $(MU_CU_OBJ)

# All executables we generate in the end
ALL_EXECUTABLES = GameOfLife.mo.co.out GameOfLife.mu.co.out GameOfLife.mo.cu.out GameOfLife.mu.cu.out

all: $(ALL_EXECUTABLES)

# arguments for the executables
CMD_ARGS = 50 50 3000 0

# Execute all executables to compare them
.PHONY: bench
bench: all
	./GameOfLife.mo.co.out $(CMD_ARGS)
	./GameOfLife.mu.co.out $(CMD_ARGS)
	./GameOfLife.mo.cu.out $(CMD_ARGS)
	./GameOfLife.mu.cu.out $(CMD_ARGS)

.PHONY: clean
clean:
	rm -f -v $(OPT_OBJ) $(UNOPT_OBJ) $(ALL_EXECUTABLES)