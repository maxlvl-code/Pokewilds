CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -O2 -Iinclude

.PHONY: test clean

test: build/world_test
	./build/world_test

build/world_test: src/pokewilds/world.c tests/world_test.c include/pokewilds/world.h
	mkdir -p build
	$(CC) $(CFLAGS) src/pokewilds/world.c tests/world_test.c -o $@

clean:
	rm -rf build
