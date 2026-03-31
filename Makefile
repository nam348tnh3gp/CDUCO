# Detect architecture
ARCH ?= $(shell uname -m)

# C compiler flags tối ưu cho Android/iOS
CFLAGS_BASE = -Wall -O3 -pthread -flto -ffast-math -fomit-frame-pointer

# ARM NEON optimization
ifeq ($(ARCH), armv7l)
    CFLAGS = $(CFLAGS_BASE) -march=armv7-a -mfpu=neon -mfloat-abi=hard -D__ARM_NEON
else ifeq ($(ARCH), aarch64)
    CFLAGS = $(CFLAGS_BASE) -march=armv8-a+crypto -mtune=cortex-a53 -D__ARM_NEON
else
    CFLAGS = $(CFLAGS_BASE) -march=native
endif

LDFLAGS = -lm

TARGET = ducominer

all: $(TARGET)

$(TARGET): main.c dsha1_opt.h
	$(CC) $(CFLAGS) -o $(TARGET) main.c $(LDFLAGS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
