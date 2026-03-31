# Simple Makefile for Duino-Coin Miner
CC = clang
CFLAGS = -Wall -O3 -pthread
LDFLAGS = -lm -pthread

all: miner

miner: main.c DSHA1.h
	$(CC) $(CFLAGS) -std=c99 -o miner main.c $(LDFLAGS)

debug: main.c DSHA1.h
	$(CC) -Wall -g -O0 -DDEBUG -std=c99 -o miner_debug main.c $(LDFLAGS)

clean:
	rm -f miner miner_debug

run: miner
	./miner

.PHONY: all debug clean run
