# Compiler - iOS simulator bắt buộc dùng clang
CC = clang
CLANG = clang

# Phát hiện hệ điều hành và kiến trúc
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Cờ tối ưu cơ bản
CFLAGS = -O3 -pthread -flto -ffast-math -fomit-frame-pointer -D_GNU_SOURCE
LDFLAGS = -lm

# iOS Simulator (i686/x86_64) hoặc iOS Device (arm64)
ifeq ($(UNAME_S), Darwin)
    # Trên macOS, dùng CommonCrypto
    CFLAGS += -DUSE_COMMON_CRYPTO
    LDFLAGS += -framework Security -framework Foundation
    CC = clang
    
    # Phát hiện nếu đang build cho iOS simulator
    ifeq ($(IOS_SIM), 1)
        # Build cho iOS simulator (i686)
        CFLAGS += -arch i386 -mios-simulator-version-min=12.0
        TARGET = ducominer-sim
    else ifeq ($(IOS_DEVICE), 1)
        # Build cho iOS device (arm64)
        CFLAGS += -arch arm64 -mios-version-min=12.0
        TARGET = ducominer-device
    else
        # macOS native
        CFLAGS += -arch x86_64 -arch arm64
        TARGET = ducominer
    endif
else
    # Linux/Android: dùng OpenSSL
    CFLAGS += -DUSE_OPENSSL
    LDFLAGS += -lcrypto
    
    # Tối ưu architecture
    ifeq ($(UNAME_M), armv7l)
        CFLAGS += -march=armv7-a -mfpu=neon -mfloat-abi=hard
    endif
    ifeq ($(UNAME_M), aarch64)
        CFLAGS += -march=armv8-a+crypto -mtune=cortex-a76
    endif
    ifeq ($(UNAME_M), x86_64)
        CFLAGS += -march=x86-64 -mtune=skylake -msse4.2 -maes
    endif
    ifeq ($(UNAME_M), i686)
        CFLAGS += -march=i686 -mtune=core2 -msse3
    endif
    
    TARGET = ducominer
endif

# Thêm warning flags
CFLAGS += -Wall -Wextra -Wno-unused-parameter

# Tối ưu link time
LDFLAGS += -flto

all: $(TARGET)

# Target cho iOS simulator (i686)
ios-sim:
	$(CC) $(CFLAGS) -arch i386 -mios-simulator-version-min=12.0 -o ducominer-sim main.c $(LDFLAGS)
	@echo "✅ Build iOS simulator thành công: ducominer-sim"
	@echo "📊 Kích thước: $$(ls -lh ducominer-sim | awk '{print $$5}')"

# Target cho iOS simulator (x86_64)
ios-sim64:
	$(CC) $(CFLAGS) -arch x86_64 -mios-simulator-version-min=12.0 -o ducominer-sim64 main.c $(LDFLAGS)
	@echo "✅ Build iOS simulator 64-bit thành công: ducominer-sim64"

# Target cho iOS device (arm64)
ios-device:
	$(CC) $(CFLAGS) -arch arm64 -mios-version-min=12.0 -isysroot $(shell xcrun --sdk iphoneos --show-sdk-path) -o ducominer-device main.c $(LDFLAGS)
	@echo "✅ Build iOS device thành công: ducominer-device"

# Target cho macOS native
macos:
	$(CC) $(CFLAGS) -arch x86_64 -arch arm64 -mmacosx-version-min=10.13 -o ducominer main.c $(LDFLAGS)
	@echo "✅ Build macOS thành công: ducominer"

# Target cho Android ARM64 (dùng cross-compiler nếu có)
android-arm64:
	$(CC) $(CFLAGS) -march=armv8-a+crypto -static -o ducominer-android main.c $(LDFLAGS)
	@echo "✅ Build Android thành công: ducominer-android"

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

# Dọn dẹp
clean:
	rm -f ducominer ducominer-sim ducominer-sim64 ducominer-device ducominer-android
	rm -f *.o *.gcda *.gcno *.gcov

# Hiển thị thông tin
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

# Chạy iOS simulator
run-sim: ios-sim
	./ducominer-sim

# Chạy iOS simulator 64-bit
run-sim64: ios-sim64
	./ducominer-sim64

.PHONY: all clean install run info ios-sim ios-sim64 ios-device macos android-arm64 debug profile sanitize benchmark valgrind run-sim run-sim64
