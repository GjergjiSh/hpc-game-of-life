PRJ_PATH = -I./include \
           -I.

OBJ_PATH = ./obj

OBJ_PRJ = $(OBJ_PATH)/GameOfLife.o $(OBJ_PATH)/Main.o

LIB_PRJ = #evtl. zusätliche libs

SRC = ./src

CC = g++ -fopenmp -std=c++17 $(PRJ_PATH) -Wno-write-strings -O3 -DNDEBUG -c

LD = g++ -O3 -DNDEBUG -Wl,--no-as-needed -o GameOfLife

all: $(OBJ_PATH) GameOfLife

$(OBJ_PATH):
	mkdir -p $(OBJ_PATH)

GameOfLife: $(OBJ_PRJ)
	$(LD) $(OBJ_PRJ) $(LIB_PRJ)

$(OBJ_PATH)/Main.o: ${SRC}/Main.cpp
	$(CC) -o $@ $?

$(OBJ_PATH)/GameOfLife.o: ${SRC}/GameOfLife.cpp
	$(CC) -o $@ $?

clean:
	rm -f -v -R $(OBJ_PATH)
	rm -f -v GameOfLife