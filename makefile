
#variables
CC = gcc
CFLAGS = -Wall -O3 -ggdb -fopenmp -latomic
TARGET = solver.exe


BUILD_DIR := build

# Recursively Find all .c files
# On Windows, we use the cmd 'dir /s /b' to get a clean list of file paths
#SRCS := $(shell dir /s /b *.c)
#SRCS := user.c algos/solver/games/quoridor.c algos/solver/games/quoridor_pathfind.c algos/solver/games/c4.c algos/solver/games/ttt.c algos/solver/games/nim.c algos/solver/play.c algos/solver/clock.c algos/solver/play_windows.c algos/solver/solver.c ds/tree.c ds/vector.c ds/list.c ds/hashmap.c algos/solver/zobrist.c misc/timing.c memory/slab.c memory/block.c memory/alloc.c misc/menu.c misc/winterm.c misc/windowing.c misc/printnum.c

#ridiculous solution but hey it works
#get all .c files, including nested directories
SRCS := $(wildcard *.c) $(wildcard **/*.c) $(wildcard **/**/*.c) $(wildcard **/**/**/*.c) $(wildcard **/**/**/**/*.c) $(wildcard **/**/**/**/**/*.c)


# Map source files to object/dep files in the build directory
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS := $(SRCS:%.c=$(BUILD_DIR)/%.d)
#OBJS := $(patsubst %.c, $(BUILD_DIR)/%.o, $(notdir $(SRCS)))

# default target
all: $(TARGET)

# linking rule
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -latomic

# compiling rule for nested directories
$(BUILD_DIR)/%.o: %.c
	@if not exist "$(@D)\" mkdir "$(@D)"
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

# clean rule
clean:
	@rmdir /s /q $(BUILD_DIR)

prof: CFLAGS += -pg
prof: $(TARGET)

release: CFLAGS += -DNDEBUG
release: $(TARGET)

.PHONY: all clean prof release

#target:
#	$(CC) $(SRCS) $(CFLAGS) -o $(TARGET)
#$(CC) -fopenmp user.c algos/solver/games/quoridor.c algos/solver/games/quoridor_pathfind.c algos/solver/games/c4.c algos/solver/games/ttt.c algos/solver/games/nim.c algos/solver/play.c algos/solver/clock.c algos/solver/play_windows.c algos/solver/solver.c ds/tree.c ds/vector.c ds/list.c ds/hashmap.c algos/solver/zobrist.c misc/timing.c memory/slab.c memory/block.c memory/alloc.c misc/menu.c misc/winterm.c misc/windowing.c misc/printnum.c $(CFLAGS) -o $(TARGET)
