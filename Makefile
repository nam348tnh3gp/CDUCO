CC = gcc
CFLAGS = -Wall -O2 -pthread
LDFLAGS = -lssl -lcrypto -lm

TARGET = ducominer

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c $(LDFLAGS)

clean:
	rm -f $(TARGET)
