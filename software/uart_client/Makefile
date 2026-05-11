CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -g -Iinclude
SRCS    = main.c src/serial.c src/protocol.c
TARGET  = rmuc

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
