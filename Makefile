CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c17
TARGET = http_server
SRC = main.c
OBJ = $(SRC:.c=.o) # Automatically generate object file names

.PHONY: all clean debug

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

debug: CFLAGS += -g -DDEBUG # Add -g for debugging info, -DDEBUG for conditional debug code
debug: all

# Example of a 'run' target for convenience (not strictly part of build)
run: $(TARGET)
	./$(TARGET)