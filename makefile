# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude # -I tells the compiler to look in the directory afterwards for headers
LDFLAGS = -lncurses -lpthread

# Source and object files
SRCS = src/main.c src/pantalla.c src/cola.c src/utils.c src/vehiculo.c
OBJS = $(SRCS:.c=.o) # Convert .c to .o automatically

# Target executable
TARGET = build/semaforo

# Arguments for the program
ARGS = 123

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Rule for compiling .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJS) $(TARGET)

# Run target
run: $(TARGET)
	./$(TARGET) $(ARGS)
