PRJ_PATH = -I./include/optimized \
           -I./include/unoptimized \
           -I.

UNOPT_OBJ_PATH = ./objunopt
OPT_OBJ_PATH = ./objopt


UNOPT_OBJ_PRJ = $(UNOPT_OBJ_PATH)/UnoptGameOfLife.o $(UNOPT_OBJ_PATH)/Main.o
OPT_OBJ_PRJ = $(OPT_OBJ_PATH)/OptGameOfLife.o $(OPT_OBJ_PATH)/Main.o


LIB_PRJ = #evtl. zusätliche libs

UNOPT_SRC = ./src/unoptimized
OPT_SRC = ./src/optimized


UNOPT_CC = g++ -std=c++17 $(PRJ_PATH) -Wno-write-strings -DNDEBUG -c
OPT_CC = g++ -fopenmp -std=c++17 $(PRJ_PATH) -Wno-write-strings -O3 -DNDEBUG -c

UNOPT_LD = g++ -DNDEBUG -Wl,--no-as-needed -o UnoptGameOfLife
OPT_LD = g++ -O3 -DNDEBUG -Wl,--no-as-needed -o OptGameOfLife

all: $(UNOPT_OBJ_PATH) UnoptGameOfLife $(OPT_OBJ_PATH) OptGameOfLife

$(UNOPT_OBJ_PATH):
	mkdir -p $(UNOPT_OBJ_PATH)

$(OPT_OBJ_PATH):
	mkdir -p $(OPT_OBJ_PATH)

UnoptGameOfLife: $(UNOPT_OBJ_PRJ)
	$(UNOPT_LD) $(UNOPT_OBJ_PRJ) $(LIB_PRJ)

$(UNOPT_OBJ_PATH)/Main.o: ${UNOPT_SRC}/Main.cpp
	$(UNOPT_CC) -o $@ $?

$(UNOPT_OBJ_PATH)/UnoptGameOfLife.o: ${UNOPT_SRC}/GameOfLife.cpp
	$(UNOPT_CC) -o $@ $?

OptGameOfLife: $(OPT_OBJ_PRJ)
	$(OPT_LD) $(OPT_OBJ_PRJ) $(LIB_PRJ)

$(OPT_OBJ_PATH)/Main.o: ${OPT_SRC}/OptMain.cpp
	$(OPT_CC) -o $@ $?

$(OPT_OBJ_PATH)/OptGameOfLife.o: ${OPT_SRC}/OptGameOfLife.cpp
	$(OPT_CC) -o $@ $?

clean:
	rm -f -v -R $(UNOPT_OBJ_PATH)
	rm -f -v -R $(OPT_OBJ_PATH)
	rm -f -v UnoptGameOfLife
	rm -f -v OptGameOfLife
