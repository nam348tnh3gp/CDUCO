# Makefile đơn giản cho Duino-Coin Miner
# Tương thích với iOS/macOS/Linux

# Compiler và flags
CC = clang
CXX = clang++
CFLAGS = -Wall -Wextra -O3 -pthread
CXXFLAGS = -Wall -Wextra -O3 -pthread
LDFLAGS = -lm -pthread

# Tên file đầu ra
TARGET = ducominer

# File nguồn
C_SOURCES = miner.c
CPP_SOURCES = miner.cpp

# Phát hiện OS
UNAME_S := $(shell uname -s)

# Thêm flags cho từng OS
ifeq ($(UNAME_S),Linux)
    CFLAGS += -lrt
    CXXFLAGS += -lrt
endif
ifeq ($(UNAME_S),Darwin)
    # macOS/iOS specific
    CFLAGS += -D_DARWIN_C_SOURCE
    CXXFLAGS += -D_DARWIN_C_SOURCE
endif

# Mặc định: build C version
all: c_version

# Build C version
c_version:
	@echo "🔨 Building C version..."
	$(CC) $(CFLAGS) -std=c99 -o $(TARGET)_c $(C_SOURCES) $(LDFLAGS)
	@echo "✅ Build complete: $(TARGET)_c"

# Build C++ version
cpp_version:
	@echo "🔨 Building C++ version..."
	$(CXX) $(CXXFLAGS) -std=c++11 -o $(TARGET)_cpp $(CPP_SOURCES) $(LDFLAGS)
	@echo "✅ Build complete: $(TARGET)_cpp"

# Build cả hai version
both: c_version cpp_version
	@echo "✅ Both versions built successfully!"

# Build với debug
debug:
	@echo "🔨 Building debug version..."
	$(CC) $(CFLAGS) -g -O0 -DDEBUG -std=c99 -o $(TARGET)_debug $(C_SOURCES) $(LDFLAGS)
	@echo "✅ Debug build complete: $(TARGET)_debug"

# Build release
release:
	@echo "🔨 Building release version..."
	$(CC) $(CFLAGS) -O3 -DNDEBUG -std=c99 -o $(TARGET)_release $(C_SOURCES) $(LDFLAGS)
	@echo "✅ Release build complete: $(TARGET)_release"

# Clean
clean:
	@echo "🧹 Cleaning..."
	rm -f $(TARGET)_c $(TARGET)_cpp $(TARGET)_debug $(TARGET)_release
	rm -f *.o
	@echo "✅ Clean complete!"

# Run C version
run: c_version
	@echo "🚀 Starting miner..."
	./$(TARGET)_c

# Help
help:
	@echo "Duino-Coin Miner Makefile"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build C version (default)"
	@echo "  c_version  - Build C version"
	@echo "  cpp_version - Build C++ version"
	@echo "  both       - Build both versions"
	@echo "  debug      - Build with debug symbols"
	@echo "  release    - Build optimized release"
	@echo "  clean      - Remove build files"
	@echo "  run        - Build and run C version"
	@echo "  help       - Show this help"

# Phụ thuộc
.PHONY: all c_version cpp_version both debug release clean run help
