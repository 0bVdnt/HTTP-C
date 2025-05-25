CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c17
TARGET = http_server
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) 
