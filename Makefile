# Compiler
CC = clang

# Phát hiện hệ điều hành
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Cờ tối ưu cơ bản
CFLAGS = -O3 -pthread -flto -ffast-math -fomit-frame-pointer
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
    
    # Tối ưu architecture
    ifeq ($(UNAME_M), aarch64)
        CFLAGS += -march=armv8-a+crypto
    else ifeq ($(UNAME_M), armv7l)
        CFLAGS += -march=armv7-a -mfpu=neon
    else ifeq ($(UNAME_M), x86_64)
        CFLAGS += -march=native
    endif
endif

# Thêm warning flags
CFLAGS += -Wall

# Target mặc định
all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c $(LDFLAGS)
	@echo "✅ Build thành công: $(TARGET)"
	@echo "📊 Kích thước: $$(ls -lh $(TARGET) | awk '{print $$5}')"

# Dọn dẹp
clean:
	rm -f $(TARGET)

# Chạy chương trình
run: $(TARGET)
	./$(TARGET)

# Hiển thị thông tin
info:
	@echo "OS: $(UNAME_S) | Arch: $(UNAME_M)"
	@echo "Compiler: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"

.PHONY: all clean run info
