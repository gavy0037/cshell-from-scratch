CC=gcc
CFlags= -std=c23 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -Wall -Wextra -Werror -Wno-unused-parameter -fno-asm
INCLUDES = -I./include


SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
TARGET_FILE= shell.out

all:$(TARGET_FILE)

$(TARGET_FILE) : $(OBJ)
	$(CC) $(CFlags) -o $(TARGET_FILE) $^

src/%.o: src/%.c
	$(CC) $(CFlags) $(INCLUDES) -c $< -o $@

run: all
	./shell.out

clean:
	rm -f src/*.o $(TARGET_FILE)