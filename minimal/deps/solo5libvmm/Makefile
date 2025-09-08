CC = gcc
CFLAGS = -Wall -Wextra -Iinclude \
		-I/../microkit-sdk-2.0.1/board/rpi4b_1gb/debug/include/microkit.h

SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
TARGET = solo5libvmm

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@

clean:
	rm -f $(OBJ) $(TARGET)