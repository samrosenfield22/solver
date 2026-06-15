
#variables
CC = gcc
CFLAGS = -Wall -O3 -ggdb -fopenmp
TARGET = user.exe


BUILD_DIR := build

# Recursively Find all .c files
# On Windows, we use the cmd 'dir /s /b' to get a clean list of file paths
#SRCS := $(shell dir /s /b *.c)
SRCS := user.c algos/solver/games/quoridor.c algos/solver/games/quoridor_pathfind.c algos/solver/games/c4.c algos/solver/games/ttt.c algos/solver/games/nim.c algos/solver/play.c algos/solver/clock.c algos/solver/play_windows.c algos/solver/solver.c ds/tree.c ds/vector.c ds/list.c ds/hashmap.c algos/solver/zobrist.c misc/timing.c memory/slab.c memory/block.c memory/alloc.c misc/menu.c misc/winterm.c misc/windowing.c misc/printnum.c
#SRCS := $(wildcard *.c) $(wildcard **/*.c)

# Map source files to object files in the build directory
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
#OBJS := $(patsubst %.c, $(BUILD_DIR)/%.o, $(notdir $(SRCS)))

# Default target
all: $(TARGET)

# 6. Linking rule
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# 7. Compiling rule for nested directories
$(BUILD_DIR)/%.o: %.c
	@if not exist "$(@D)\" mkdir "$(@D)"
	$(CC) $(CFLAGS) -c $< -o $@

# 8. Clean rule
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean

#target:
#	$(CC) $(SRCS) $(CFLAGS) -o $(TARGET)
#$(CC) -fopenmp user.c algos/solver/games/quoridor.c algos/solver/games/quoridor_pathfind.c algos/solver/games/c4.c algos/solver/games/ttt.c algos/solver/games/nim.c algos/solver/play.c algos/solver/clock.c algos/solver/play_windows.c algos/solver/solver.c ds/tree.c ds/vector.c ds/list.c ds/hashmap.c algos/solver/zobrist.c misc/timing.c memory/slab.c memory/block.c memory/alloc.c misc/menu.c misc/winterm.c misc/windowing.c misc/printnum.c $(CFLAGS) -o $(TARGET)
