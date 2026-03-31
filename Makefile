CC = gcc
CFLAGS = -Wall -O2 -pthread
LDFLAGS = -lm

TARGET = ducominer

all: $(TARGET)

$(TARGET): main.c dsha1.h
	$(CC) $(CFLAGS) -o $(TARGET) main.c $(LDFLAGS)

clean:
	rm -f $(TARGET)
