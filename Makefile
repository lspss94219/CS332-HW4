# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g

# Target executable
TARGET = traverse

# Source files
SRC = traverse.c arguments.c file_operations.c sort.c

# Object files (generated from source files)
OBJ = $(SRC:.c=.o)

# Header files
HEADERS = arguments.h file_operations.h sort.h

# Default rule to build the executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) -lws2_32

# Rule to compile .c files into .o files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and the executable
clean:
	rm -f $(OBJ) $(TARGET)

# Convenience rule to run the program
run: $(TARGET)
	./$(TARGET)

# Phony targets
.PHONY: clean run
