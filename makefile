# Compiler and flags
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g -Iinclude # -I tells the compiler to look in the directory afterwards for headers
LDFLAGS = -lncurses -lpthread
BUILD_DIR = build

# Source and object files
SRCS = src/main.c src/pantalla.c src/cola.c src/utils.c src/vehiculo.c
OBJS = $(SRCS:.c=.o) # Convert .c to .o automatically
# $(patsubst pattern,replacement,text)
# patsubst: finds whitespace-separated words in text that match pattern and replaces them with replacement
# it goes through the elements in SRCS and replaces src/*.c with build/*.o
OBJS = $(patsubst src/%.c, $(BUILD_DIR)/%.o, $(SRCS)) # Place .o files in BUILD_DIR

# Target executable
TARGET = $(BUILD_DIR)/semaforo

# Arguments for the program
ARGS = 123

# Default target
all: $(TARGET)

# Build the target
# 1) $(BUILD_DIR) -> makes sure the build dir exists
# 2) compiling: $(OBJS) -> copies the src/*.c files to the build/*.o files and compiles the objects using the rule $(BUILD_DIR)/%.o: %.c
# 3) linking: compiled object files are linked to the final target executable
$(TARGET): $(BUILD_DIR) $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
# $@ refers to the $(TARGET)

# Rule for compiling .c to .o
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR) # rule to compile .c to .o | ensures the build folder is created
	$(CC) $(CFLAGS) -c $< -o $@
# $@ refers to  %.o, $< refers to the first prerequisite in the rule

# Rule for build dir
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Clean target
clean:
	rm -f $(OBJS) $(TARGET)
	rm -f logs/*.log

# Run target
run: $(TARGET)
	./$(TARGET) $(ARGS)
