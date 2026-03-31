CC = gcc
CFLAGS = -Wall -O3 -pthread -flto -ffast-math -fomit-frame-pointer -D_GNU_SOURCE
LDFLAGS = -lm

# Phát hiện architecture và bật NEON nếu có
ARCH := $(shell uname -m)
ifeq ($(ARCH), armv7l)
    CFLAGS += -march=armv7-a -mfpu=neon -mfloat-abi=hard
endif
ifeq ($(ARCH), aarch64)
    CFLAGS += -march=armv8-a+crypto
endif

TARGET = ducominer

all: $(TARGET)

$(TARGET): main.c dsha1.h
	$(CC) $(CFLAGS) -o $(TARGET) main.c $(LDFLAGS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

run: $(TARGET)
	./$(TARGET)
