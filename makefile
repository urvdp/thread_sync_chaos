# Compiler and flags
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g -Iinclude # -I tells the compiler to look in the directory afterwards for headers
LDFLAGS = -lncurses -lpthread
BUILD_DIR = build

# Source and object files
SRCS = src/main.c src/pantalla.c src/cola.c src/utils.c src/vehiculo.c
OBJS = $(SRCS:.c=.o) # Convert .c to .o automatically

# Target executable
TARGET = $(BUILD_DIR)/semaforo

# Arguments for the program
ARGS = 123

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(BUILD_DIR) $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Rule for compiling .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for build dir
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean target
clean:
	rm -f $(OBJS) $(TARGET)

# Run target
run: $(TARGET)
	./$(TARGET) $(ARGS)
