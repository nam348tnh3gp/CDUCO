# Makefile cho Duino-Coin Miner
# Hỗ trợ cả C và C++ version

# Compiler và flags
CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -O3 -march=native -flto -pthread
CXXFLAGS = -Wall -Wextra -O3 -march=native -flto -pthread
LDFLAGS = -lm -pthread

# Tên file đầu ra
TARGET = duino_miner

# File nguồn
C_SOURCES = miner.c
CPP_SOURCES = miner.cpp

# Header files
HEADERS = DSHA1.h

# Phát hiện OS
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LDFLAGS += -lrt
endif
ifeq ($(UNAME_S),Darwin)
    LDFLAGS += 
endif

# Mặc định: sử dụng C version
all: c_version

# Build C version (dùng DSHA1.h C version)
c_version: CFLAGS += -std=c99
c_version: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building C version..."
	$(CC) $(CFLAGS) -o $(TARGET)_c $(C_SOURCES) $(LDFLAGS)
	@echo "✅ Build complete: $(TARGET)_c"

# Build C++ version (dùng DSHA1.h C++ version)
cpp_version: CXXFLAGS += -std=c++11
cpp_version: $(CPP_SOURCES) $(HEADERS)
	@echo "🔨 Building C++ version..."
	$(CXX) $(CXXFLAGS) -o $(TARGET)_cpp $(CPP_SOURCES) $(LDFLAGS)
	@echo "✅ Build complete: $(TARGET)_cpp"

# Build cả hai version
both: c_version cpp_version
	@echo "✅ Both versions built successfully!"

# Build với debug symbols
debug: CFLAGS += -g -O0 -DDEBUG -Wall -Wextra
debug: CXXFLAGS += -g -O0 -DDEBUG -Wall -Wextra
debug: c_version_debug cpp_version_debug

c_version_debug: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building C version with debug symbols..."
	$(CC) $(CFLAGS) -o $(TARGET)_c_debug $(C_SOURCES) $(LDFLAGS)
	@echo "✅ Debug build complete: $(TARGET)_c_debug"

cpp_version_debug: $(CPP_SOURCES) $(HEADERS)
	@echo "🔨 Building C++ version with debug symbols..."
	$(CXX) $(CXXFLAGS) -o $(TARGET)_cpp_debug $(CPP_SOURCES) $(LDFLAGS)
	@echo "✅ Debug build complete: $(TARGET)_cpp_debug"

# Build với tối ưu hóa cao nhất (release)
release: CFLAGS += -O3 -march=native -flto -DNDEBUG
release: CXXFLAGS += -O3 -march=native -flto -DNDEBUG
release: c_version_release cpp_version_release

c_version_release: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building C release version..."
	$(CC) $(CFLAGS) -o $(TARGET)_c_release $(C_SOURCES) $(LDFLAGS)
	@echo "✅ Release build complete: $(TARGET)_c_release"

cpp_version_release: $(CPP_SOURCES) $(HEADERS)
	@echo "🔨 Building C++ release version..."
	$(CXX) $(CXXFLAGS) -o $(TARGET)_cpp_release $(CPP_SOURCES) $(LDFLAGS)
	@echo "✅ Release build complete: $(TARGET)_cpp_release"

# Build với profile optimization
profile: CFLAGS += -O3 -march=native -flto -fprofile-generate
profile: CXXFLAGS += -O3 -march=native -flto -fprofile-generate
profile: c_version_profile cpp_version_profile

c_version_profile: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building C version with profile generation..."
	$(CC) $(CFLAGS) -o $(TARGET)_c_profile $(C_SOURCES) $(LDFLAGS)
	@echo "✅ Profile build complete: $(TARGET)_c_profile"
	@echo "📊 Run the program to generate profile data, then rebuild with 'profile-use'"

cpp_version_profile: $(CPP_SOURCES) $(HEADERS)
	@echo "🔨 Building C++ version with profile generation..."
	$(CXX) $(CXXFLAGS) -o $(TARGET)_cpp_profile $(CPP_SOURCES) $(LDFLAGS)
	@echo "✅ Profile build complete: $(TARGET)_cpp_profile"
	@echo "📊 Run the program to generate profile data, then rebuild with 'profile-use'"

# Build sử dụng profile data (sau khi đã chạy profile version)
profile-use: CFLAGS += -O3 -march=native -flto -fprofile-use
profile-use: CXXFLAGS += -O3 -march=native -flto -fprofile-use
profile-use: c_version_profile_use cpp_version_profile_use

c_version_profile_use: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building C version with profile optimization..."
	$(CC) $(CFLAGS) -o $(TARGET)_c_optimized $(C_SOURCES) $(LDFLAGS)
	@echo "✅ Profile-optimized build complete: $(TARGET)_c_optimized"

cpp_version_profile_use: $(CPP_SOURCES) $(HEADERS)
	@echo "🔨 Building C++ version with profile optimization..."
	$(CXX) $(CXXFLAGS) -o $(TARGET)_cpp_optimized $(CPP_SOURCES) $(LDFLAGS)
	@echo "✅ Profile-optimized build complete: $(TARGET)_cpp_optimized"

# Build cho Raspberry Pi / ARM
arm: CC = arm-linux-gnueabihf-gcc
arm: CXX = arm-linux-gnueabihf-g++
arm: CFLAGS += -std=c99 -O3 -march=armv7-a -mfpu=neon -mfloat-abi=hard
arm: CXXFLAGS += -std=c++11 -O3 -march=armv7-a -mfpu=neon -mfloat-abi=hard
arm: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building for ARM (Raspberry Pi)..."
	$(CC) $(CFLAGS) -o $(TARGET)_arm $(C_SOURCES) $(LDFLAGS)
	@echo "✅ ARM build complete: $(TARGET)_arm"

# Build cho Android (Termux)
android: CC = clang
android: CXX = clang++
android: CFLAGS += -std=c99 -O3 -march=armv8-a -D__ANDROID__
android: CXXFLAGS += -std=c++11 -O3 -march=armv8-a -D__ANDROID__
android: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building for Android..."
	$(CC) $(CFLAGS) -o $(TARGET)_android $(C_SOURCES) $(LDFLAGS)
	@echo "✅ Android build complete: $(TARGET)_android"

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	rm -f $(TARGET)_c $(TARGET)_cpp $(TARGET)_c_debug $(TARGET)_cpp_debug
	rm -f $(TARGET)_c_release $(TARGET)_cpp_release
	rm -f $(TARGET)_c_profile $(TARGET)_cpp_profile
	rm -f $(TARGET)_c_optimized $(TARGET)_cpp_optimized
	rm -f $(TARGET)_arm $(TARGET)_android
	rm -f *.o *.gcda *.gcno *.gcov
	@echo "✅ Clean complete!"

# Install (copy to /usr/local/bin)
install: c_version_release
	@echo "📦 Installing to /usr/local/bin/..."
	sudo cp $(TARGET)_c_release /usr/local/bin/$(TARGET)
	sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "✅ Installed! Run '$(TARGET)' to start mining"

# Uninstall
uninstall:
	@echo "🗑️ Uninstalling..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "✅ Uninstalled!"

# Run the miner (C version)
run: c_version
	@echo "🚀 Starting miner (C version)..."
	./$(TARGET)_c

# Run with valgrind (memory check)
valgrind: debug
	@echo "🔍 Running valgrind check..."
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)_c_debug

# Run with gdb (debugger)
gdb: debug
	@echo "🐛 Starting GDB debugger..."
	gdb ./$(TARGET)_c_debug

# Check code style
style:
	@echo "📝 Checking code style..."
	@if command -v indent > /dev/null; then \
		indent -linux -bad -bap -nbc -bbo -hnl -br -brs -c33 -cd33 -ncdb -ce -ci4 -cli0 -d0 -di1 -nfc1 -i4 -ip0 -l80 -lp -npcs -nprs -npsl -sai -saf -saw -ncs -nsc -sob -nfca -cp33 -ss -ts4 miner.c; \
		echo "✅ Code style checked!"; \
	else \
		echo "⚠️ indent not installed. Run: sudo apt-get install indent"; \
	fi

# Show help
help:
	@echo "Duino-Coin Miner Makefile"
	@echo ""
	@echo "Usage: make [target]"
	@echo ""
	@echo "Build targets:"
	@echo "  all           Build C version (default)"
	@echo "  c_version     Build C version with DSHA1.h C"
	@echo "  cpp_version   Build C++ version with DSHA1.h C++"
	@echo "  both          Build both versions"
	@echo "  debug         Build with debug symbols"
	@echo "  release       Build optimized release version"
	@echo "  profile       Build with profiling support"
	@echo "  profile-use   Build with profile optimization"
	@echo "  arm           Build for ARM (Raspberry Pi)"
	@echo "  android       Build for Android (Termux)"
	@echo ""
	@echo "Utility targets:"
	@echo "  clean         Remove build artifacts"
	@echo "  install       Install to /usr/local/bin"
	@echo "  uninstall     Remove from /usr/local/bin"
	@echo "  run           Run C version"
	@echo "  valgrind      Memory check with valgrind"
	@echo "  gdb           Debug with GDB"
	@echo "  style         Check code style"
	@echo "  help          Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make c_version          # Build C version"
	@echo "  make clean && make      # Clean and build"
	@echo "  make release            # Build optimized version"
	@echo "  make install            # Install system-wide"

# Phát hiện số lượng CPU cores cho parallel build
CORES := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
MAKEFLAGS += -j$(CORES)

# Phụ thuộc
.PHONY: all c_version cpp_version both debug release profile profile-use arm android clean install uninstall run valgrind gdb style help
.PHONY: c_version_debug cpp_version_debug c_version_release cpp_version_release
.PHONY: c_version_profile cpp_version_profile c_version_profile_use cpp_version_profile_use

# Thông báo
.SILENT: clean
