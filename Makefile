CC=gcc
CFlags= -std=c23 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -Wall -Wextra -Werror -Wno-unused-parameter -fno-asm
INCLUDES = -I./include


SRC=$(wildcard src/*.c)
OBJ=$(patsubst src/%.c, build/%.o, $(SRC))
TARGET_FILE= shell.out

all: $(TARGET_FILE)

$(TARGET_FILE) : $(OBJ)
	$(CC) $(CFlags) -o $(TARGET_FILE) $^

build/%.o: src/%.c | build
	$(CC) $(CFlags) $(INCLUDES) -c $< -o $@

build:
	mkdir -p build

run: all
	./shell.out

clean:
	rm -rf build $(TARGET_FILE)