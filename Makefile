CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -O2 -Iinclude
CORE = src/pokewilds/world.c src/pokewilds/game.c

.PHONY: all test play clean
all: test build/pokewilds-prototype

test: build/world_test build/game_test
	./build/world_test
	./build/game_test

build/world_test: $(CORE) tests/world_test.c include/pokewilds/world.h include/pokewilds/game.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/world_test.c -o $@

build/game_test: $(CORE) tests/game_test.c include/pokewilds/world.h include/pokewilds/game.h
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tests/game_test.c -o $@

build/pokewilds-prototype: $(CORE) tools/playable_cli.c
	mkdir -p build
	$(CC) $(CFLAGS) $(CORE) tools/playable_cli.c -o $@

play: build/pokewilds-prototype
	./build/pokewilds-prototype

clean:
	rm -rf build
