# Simple Makefile for Duino-Coin Miner
CC = clang
CFLAGS = -Wall -O3 -pthread
LDFLAGS = -lm -pthread

all: miner

miner: miner.c DSHA1.h
	$(CC) $(CFLAGS) -std=c99 -o miner miner.c $(LDFLAGS)

debug: miner.c DSHA1.h
	$(CC) -Wall -g -O0 -DDEBUG -std=c99 -o miner_debug miner.c $(LDFLAGS)

clean:
	rm -f miner miner_debug

run: miner
	./miner

help:
	@echo "Commands:"
	@echo "  make     - Build miner"
	@echo "  make debug - Build debug version"
	@echo "  make clean - Remove binaries"
	@echo "  make run   - Build and run"

.PHONY: all debug clean run help
