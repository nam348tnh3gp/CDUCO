# Compiler
CC = clang

# Phát hiện hệ điều hành và kiến trúc
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Cờ tối ưu cơ bản (bỏ -flto để tránh lỗi)
CFLAGS = -O3 -pthread -ffast-math -fomit-frame-pointer
LDFLAGS = -lm

# Cấu hình theo hệ điều hành
ifeq ($(UNAME_S), Darwin)
    # iOS/macOS: dùng CommonCrypto
    CFLAGS += -DUSE_COMMON_CRYPTO
    LDFLAGS += -framework Security -framework Foundation
    TARGET = ducominer
    
    # Thêm cờ cho từng kiến trúc
    ifeq ($(UNAME_M), i686)
        CFLAGS += -arch i386 -mios-simulator-version-min=12.0
    else ifeq ($(UNAME_M), x86_64)
        CFLAGS += -arch x86_64 -mios-simulator-version-min=12.0
    else ifeq ($(UNAME_M), arm64)
        CFLAGS += -arch arm64 -miphoneos-version-min=12.0
    else
        # macOS native
        CFLAGS += -arch x86_64 -arch arm64
    endif
else
    # Android/Linux: dùng OpenSSL
    CFLAGS += -DUSE_OPENSSL
    LDFLAGS += -lcrypto
    TARGET = ducominer
    
    # Tối ưu architecture (không dùng -flto)
    ifeq ($(UNAME_M), aarch64)
        CFLAGS += -march=armv8-a+crypto -mtune=cortex-a76
    else ifeq ($(UNAME_M), armv7l)
        CFLAGS += -march=armv7-a -mfpu=neon -mfloat-abi=hard
    else ifeq ($(UNAME_M), x86_64)
        CFLAGS += -march=native
    else ifeq ($(UNAME_M), i686)
        CFLAGS += -march=i686 -mtune=core2
    endif
endif

# Thêm warning flags (giảm bớt để tránh cảnh báo không cần thiết)
CFLAGS += -Wall

# Target mặc định
all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c $(LDFLAGS)
	@echo "✅ Build thành công: $(TARGET)"
	@echo "📊 Kích thước: $$(ls -lh $(TARGET) | awk '{print $$5}')"
	@echo "🔧 Kiến trúc: $$(file $(TARGET) | cut -d: -f2)"

# Build với debug (không tối ưu, dễ debug)
debug: CFLAGS = -O0 -g -pthread -Wall -DUSE_OPENSSL
debug: $(TARGET)

# Build với tối ưu cao nhất (nếu muốn thử lại -flto)
extreme: CFLAGS += -O3 -funroll-loops -fprefetch-loop-arrays
extreme: $(TARGET)

# Dọn dẹp
clean:
	rm -f $(TARGET)

# Chạy chương trình
run: $(TARGET)
	./$(TARGET)

# Hiển thị thông tin
info:
	@echo "=== Thông tin build ==="
	@echo "OS: $(UNAME_S)"
	@echo "Arch: $(UNAME_M)"
	@echo "Compiler: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "======================"

.PHONY: all clean run info debug extreme
