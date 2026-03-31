# Compiler
CC = gcc
CLANG = clang

# Phát hiện hệ điều hành
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Cờ tối ưu cơ bản
CFLAGS = -O3 -pthread -flto -ffast-math -fomit-frame-pointer -D_GNU_SOURCE
LDFLAGS = -lm

# Kiểm tra nếu là macOS/iOS
ifeq ($(UNAME_S), Darwin)
    # Dùng CommonCrypto (Apple native)
    CFLAGS += -DUSE_COMMON_CRYPTO
    LDFLAGS += -framework Security -framework Foundation
    
    # Clang tối ưu hơn GCC trên macOS
    CC = clang
    CFLAGS += -arch arm64 -arch x86_64 -mmacosx-version-min=10.13
else
    # Linux/Android: dùng OpenSSL
    CFLAGS += -DUSE_OPENSSL
    LDFLAGS += -lcrypto
endif

# Tối ưu architecture
ifeq ($(UNAME_M), armv7l)
    CFLAGS += -march=armv7-a -mfpu=neon -mfloat-abi=hard
endif

ifeq ($(UNAME_M), aarch64)
    CFLAGS += -march=armv8-a+crypto -mtune=cortex-a76
endif

ifeq ($(UNAME_M), arm64)
    # Apple Silicon
    CFLAGS += -arch arm64 -mtune=apple-m1
endif

ifeq ($(UNAME_M), x86_64)
    CFLAGS += -march=x86-64 -mtune=skylake -msse4.2 -maes
endif

ifeq ($(UNAME_M), i686)
    CFLAGS += -march=i686 -mtune=core2 -msse3
endif

# Thêm warning flags
CFLAGS += -Wall -Wextra -Wno-unused-parameter

# Tối ưu link time
LDFLAGS += -flto

TARGET = ducominer

# Target cho iOS simulator (i686)
ios-sim:
	$(CC) $(CFLAGS) -arch i386 -mios-simulator-version-min=12.0 -o $(TARGET)-sim main.c $(LDFLAGS)

# Target cho iOS device (arm64)
ios-device:
	$(CC) $(CFLAGS) -arch arm64 -mios-version-min=12.0 -isysroot $(shell xcrun --sdk iphoneos --show-sdk-path) -o $(TARGET)-device main.c $(LDFLAGS)

# Target cho Android (ARM64)
android-arm64:
	$(CC) $(CFLAGS) -march=armv8-a+crypto -static -o $(TARGET)-android main.c $(LDFLAGS)

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c $(LDFLAGS)
	@echo "✅ Build thành công: $(TARGET)"
	@echo "📊 Kích thước: $$(ls -lh $(TARGET) | awk '{print $$5}')"

# Build với debug info
debug: CFLAGS += -g -O0 -DDEBUG
debug: $(TARGET)

# Build với profile (tối ưu + debug symbols)
profile: CFLAGS += -g -O3 -fprofile-arcs -ftest-coverage
profile: $(TARGET)

# Build với sanitizers (phát hiện lỗi)
sanitize: CFLAGS += -fsanitize=address -fsanitize=undefined -g
sanitize: $(TARGET)

# Kiểm tra hiệu năng
benchmark: $(TARGET)
	@echo "🏃 Running benchmark..."
	time ./$(TARGET) --benchmark

# Chạy với valgrind (nếu có)
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Cài đặt
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/
	@echo "✅ Đã cài đặt vào /usr/local/bin/$(TARGET)"

# Gỡ cài đặt
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Dọn dẹp
clean:
	rm -f $(TARGET) $(TARGET)-sim $(TARGET)-device $(TARGET)-android
	rm -f *.o *.gcda *.gcno *.gcov

# Dọn dẹp hoàn toàn
distclean: clean
	rm -f config.txt

# Hiển thị cấu hình
info:
	@echo "=== Thông tin build ==="
	@echo "OS: $(UNAME_S)"
	@echo "Arch: $(UNAME_M)"
	@echo "Compiler: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "======================"

# Chạy chương trình
run: $(TARGET)
	./$(TARGET)

# Chạy với nhiều thread
run-multi: $(TARGET)
	./$(TARGET) --threads 4

.PHONY: all clean install uninstall run info ios-sim ios-device android-arm64 benchmark valgrind debug profile sanitize
