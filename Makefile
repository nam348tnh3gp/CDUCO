# Makefile cho Duino-Coin Miner
# Sử dụng Clang/Clang++ compiler

# Compiler và flags - Sử dụng Clang
CC = clang
CXX = clang++
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
    # macOS specific flags
    LDFLAGS += 
endif

# Mặc định: sử dụng C version
all: c_version

# Build C version (dùng DSHA1.h C version)
c_version: CFLAGS += -std=c99
c_version: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building C version with Clang..."
	$(CC) $(CFLAGS) -o $(TARGET)_c $(C_SOURCES) $(LDFLAGS)
	@echo "✅ Build complete: $(TARGET)_c"

# Build C++ version (dùng DSHA1.h C++ version)
cpp_version: CXXFLAGS += -std=c++11
cpp_version: $(CPP_SOURCES) $(HEADERS)
	@echo "🔨 Building C++ version with Clang++..."
	$(CXX) $(CXXFLAGS) -o $(TARGET)_cpp $(CPP_SOURCES) $(LDFLAGS)
	@echo "✅ Build complete: $(TARGET)_cpp"

# Build cả hai version
both: c_version cpp_version
	@echo "✅ Both versions built successfully!"

# Build với Address Sanitizer (phát hiện lỗi bộ nhớ)
asan: CFLAGS += -std=c99 -fsanitize=address -g -O1 -fno-omit-frame-pointer
asan: CXXFLAGS += -std=c++11 -fsanitize=address -g -O1 -fno-omit-frame-pointer
asan: LDFLAGS += -fsanitize=address
asan: c_version_asan cpp_version_asan
	@echo "🔍 Address Sanitizer enabled"

c_version_asan: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building C version with AddressSanitizer..."
	$(CC) $(CFLAGS) -o $(TARGET)_c_asan $(C_SOURCES) $(LDFLAGS)
	@echo "✅ ASAN build complete: $(TARGET)_c_asan"

cpp_version_asan: $(CPP_SOURCES) $(HEADERS)
	@echo "🔨 Building C++ version with AddressSanitizer..."
	$(CXX) $(CXXFLAGS) -o $(TARGET)_cpp_asan $(CPP_SOURCES) $(LDFLAGS)
	@echo "✅ ASAN build complete: $(TARGET)_cpp_asan"

# Build với Undefined Behavior Sanitizer
ubsan: CFLAGS += -std=c99 -fsanitize=undefined -g -O1
ubsan: CXXFLAGS += -std=c++11 -fsanitize=undefined -g -O1
ubsan: LDFLAGS += -fsanitize=undefined
ubsan: c_version_ubsan cpp_version_ubsan
	@echo "⚠️  UndefinedBehavior Sanitizer enabled"

c_version_ubsan: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building C version with UBSan..."
	$(CC) $(CFLAGS) -o $(TARGET)_c_ubsan $(C_SOURCES) $(LDFLAGS)
	@echo "✅ UBSAN build complete: $(TARGET)_c_ubsan"

cpp_version_ubsan: $(CPP_SOURCES) $(HEADERS)
	@echo "🔨 Building C++ version with UBSan..."
	$(CXX) $(CXXFLAGS) -o $(TARGET)_cpp_ubsan $(CPP_SOURCES) $(LDFLAGS)
	@echo "✅ UBSAN build complete: $(TARGET)_cpp_ubsan"

# Build với Memory Sanitizer (chỉ Linux)
msan: CFLAGS += -std=c99 -fsanitize=memory -g -O1 -fno-omit-frame-pointer -fPIE
msan: CXXFLAGS += -std=c++11 -fsanitize=memory -g -O1 -fno-omit-frame-pointer -fPIE
msan: LDFLAGS += -fsanitize=memory -fPIE -pie
msan: c_version_msan cpp_version_msan
	@echo "🧠 Memory Sanitizer enabled (Linux only)"

c_version_msan: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building C version with MemorySanitizer..."
	$(CC) $(CFLAGS) -o $(TARGET)_c_msan $(C_SOURCES) $(LDFLAGS)
	@echo "✅ MSAN build complete: $(TARGET)_c_msan"

cpp_version_msan: $(CPP_SOURCES) $(HEADERS)
	@echo "🔨 Building C++ version with MemorySanitizer..."
	$(CXX) $(CXXFLAGS) -o $(TARGET)_cpp_msan $(CPP_SOURCES) $(LDFLAGS)
	@echo "✅ MSAN build complete: $(TARGET)_cpp_msan"

# Build với Thread Sanitizer
tsan: CFLAGS += -std=c99 -fsanitize=thread -g -O1
tsan: CXXFLAGS += -std=c++11 -fsanitize=thread -g -O1
tsan: LDFLAGS += -fsanitize=thread
tsan: c_version_tsan cpp_version_tsan
	@echo "🧵 Thread Sanitizer enabled"

c_version_tsan: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building C version with ThreadSanitizer..."
	$(CC) $(CFLAGS) -o $(TARGET)_c_tsan $(C_SOURCES) $(LDFLAGS)
	@echo "✅ TSAN build complete: $(TARGET)_c_tsan"

cpp_version_tsan: $(CPP_SOURCES) $(HEADERS)
	@echo "🔨 Building C++ version with ThreadSanitizer..."
	$(CXX) $(CXXFLAGS) -o $(TARGET)_cpp_tsan $(CPP_SOURCES) $(LDFLAGS)
	@echo "✅ TSAN build complete: $(TARGET)_cpp_tsan"

# Build với debug symbols
debug: CFLAGS += -g -O0 -DDEBUG -Wall -Wextra -fno-omit-frame-pointer
debug: CXXFLAGS += -g -O0 -DDEBUG -Wall -Wextra -fno-omit-frame-pointer
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
release: CFLAGS += -O3 -march=native -flto -DNDEBUG -fomit-frame-pointer
release: CXXFLAGS += -O3 -march=native -flto -DNDEBUG -fomit-frame-pointer
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

# Build với LTO và PGO kết hợp
pgo: c_version_pgo cpp_version_pgo

c_version_pgo: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building C version with PGO (Profile Guided Optimization)..."
	@echo "Step 1: Building with profile generation..."
	$(CC) -O3 -march=native -fprofile-generate -o $(TARGET)_c_pgo_gen $(C_SOURCES) $(LDFLAGS)
	@echo "Step 2: Running to generate profile data..."
	@echo "⚠️  Please run: ./$(TARGET)_c_pgo_gen for a few minutes, then press Enter"
	@read -p "Press Enter to continue after profiling..." dummy
	@echo "Step 3: Building optimized version with profile data..."
	$(CC) -O3 -march=native -fprofile-use -o $(TARGET)_c_pgo $(C_SOURCES) $(LDFLAGS)
	@echo "✅ PGO build complete: $(TARGET)_c_pgo"
	@rm -f $(TARGET)_c_pgo_gen

cpp_version_pgo: $(CPP_SOURCES) $(HEADERS)
	@echo "🔨 Building C++ version with PGO..."
	@echo "Step 1: Building with profile generation..."
	$(CXX) -O3 -march=native -fprofile-generate -o $(TARGET)_cpp_pgo_gen $(CPP_SOURCES) $(LDFLAGS)
	@echo "Step 2: Running to generate profile data..."
	@echo "⚠️  Please run: ./$(TARGET)_cpp_pgo_gen for a few minutes, then press Enter"
	@read -p "Press Enter to continue after profiling..." dummy
	@echo "Step 3: Building optimized version with profile data..."
	$(CXX) -O3 -march=native -fprofile-use -o $(TARGET)_cpp_pgo $(CPP_SOURCES) $(LDFLAGS)
	@echo "✅ PGO build complete: $(TARGET)_cpp_pgo"
	@rm -f $(TARGET)_cpp_pgo_gen

# Build cho Raspberry Pi / ARM
arm: CC = clang --target=arm-linux-gnueabihf
arm: CXX = clang++ --target=arm-linux-gnueabihf
arm: CFLAGS += -std=c99 -O3 -march=armv7-a -mfpu=neon -mfloat-abi=hard
arm: CXXFLAGS += -std=c++11 -O3 -march=armv7-a -mfpu=neon -mfloat-abi=hard
arm: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building for ARM (Raspberry Pi) with Clang..."
	$(CC) $(CFLAGS) -o $(TARGET)_arm $(C_SOURCES) $(LDFLAGS)
	@echo "✅ ARM build complete: $(TARGET)_arm"

# Build cho Android (Termux)
android: CC = clang
android: CXX = clang++
android: CFLAGS += -std=c99 -O3 -march=armv8-a -D__ANDROID__ -fPIE
android: CXXFLAGS += -std=c++11 -O3 -march=armv8-a -D__ANDROID__ -fPIE
android: LDFLAGS += -fPIE -pie
android: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building for Android (Termux)..."
	$(CC) $(CFLAGS) -o $(TARGET)_android $(C_SOURCES) $(LDFLAGS)
	@echo "✅ Android build complete: $(TARGET)_android"

# Build cho WebAssembly (Experimental)
wasm: CC = clang --target=wasm32-unknown-wasi
wasm: CFLAGS += -std=c99 -O3 -flto -nostdlib -Wl,--no-entry
wasm: $(C_SOURCES) $(HEADERS)
	@echo "🔨 Building WebAssembly version..."
	$(CC) $(CFLAGS) -o $(TARGET).wasm $(C_SOURCES)
	@echo "✅ WebAssembly build complete: $(TARGET).wasm"

# Phân tích code với scan-build
scan:
	@echo "🔍 Running Clang Static Analyzer..."
	scan-build --use-cc=$(CC) --use-c++=$(CXX) make clean
	scan-build --use-cc=$(CC) --use-c++=$(CXX) make c_version
	@echo "✅ Static analysis complete"

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	rm -f $(TARGET)_c $(TARGET)_cpp $(TARGET)_c_debug $(TARGET)_cpp_debug
	rm -f $(TARGET)_c_release $(TARGET)_cpp_release
	rm -f $(TARGET)_c_profile $(TARGET)_cpp_profile
	rm -f $(TARGET)_c_optimized $(TARGET)_cpp_optimized
	rm -f $(TARGET)_arm $(TARGET)_android
	rm -f $(TARGET)_c_asan $(TARGET)_cpp_asan
	rm -f $(TARGET)_c_ubsan $(TARGET)_cpp_ubsan
	rm -f $(TARGET)_c_msan $(TARGET)_cpp_msan
	rm -f $(TARGET)_c_tsan $(TARGET)_cpp_tsan
	rm -f $(TARGET)_c_pgo $(TARGET)_cpp_pgo
	rm -f $(TARGET)_c_pgo_gen $(TARGET)_cpp_pgo_gen
	rm -f $(TARGET).wasm
	rm -f *.o *.gcda *.gcno *.gcov *.profraw *.profdata
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

# Run with lldb (debugger)
lldb: debug
	@echo "🐛 Starting LLDB debugger..."
	lldb ./$(TARGET)_c_debug

# Run with perf (performance profiling)
perf: release
	@echo "📊 Running performance profiling..."
	sudo perf stat ./$(TARGET)_c_release

# Check code style
style:
	@echo "📝 Checking code style with clang-format..."
	@if command -v clang-format > /dev/null; then \
		clang-format -i --style=llvm miner.c miner.cpp DSHA1.h; \
		echo "✅ Code formatted with clang-format!"; \
	else \
		echo "⚠️ clang-format not installed. Run: sudo apt-get install clang-format"; \
	fi

# Show compiler information
info:
	@echo "🔧 Compiler Information:"
	@echo "C Compiler: $(CC) ($(shell $(CC) --version | head -n1))"
	@echo "C++ Compiler: $(CXX) ($(shell $(CXX) --version | head -n1))"
	@echo "Target OS: $(UNAME_S)"
	@echo "CPU Cores: $(CORES)"

# Show help
help:
	@echo "Duino-Coin Miner Makefile (Clang version)"
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
	@echo "  pgo           Build with Profile Guided Optimization"
	@echo "  arm           Build for ARM (Raspberry Pi)"
	@echo "  android       Build for Android (Termux)"
	@echo "  wasm          Build WebAssembly version (experimental)"
	@echo ""
	@echo "Sanitizer targets (for debugging):"
	@echo "  asan          Build with AddressSanitizer"
	@echo "  ubsan         Build with UndefinedBehaviorSanitizer"
	@echo "  msan          Build with MemorySanitizer (Linux only)"
	@echo "  tsan          Build with ThreadSanitizer"
	@echo ""
	@echo "Utility targets:"
	@echo "  clean         Remove build artifacts"
	@echo "  install       Install to /usr/local/bin"
	@echo "  uninstall     Remove from /usr/local/bin"
	@echo "  run           Run C version"
	@echo "  valgrind      Memory check with valgrind"
	@echo "  lldb          Debug with LLDB"
	@echo "  perf          Performance profiling with perf"
	@echo "  scan          Run Clang Static Analyzer"
	@echo "  style         Format code with clang-format"
	@echo "  info          Show compiler information"
	@echo "  help          Show this help"
	@echo ""
	@echo "Examples:"
	@echo "  make c_version          # Build C version with Clang"
	@echo "  make release            # Build optimized version"
	@echo "  make asan               # Build with AddressSanitizer"
	@echo "  make clean && make      # Clean and build"
	@echo "  make install            # Install system-wide"

# Phát hiện số lượng CPU cores cho parallel build
CORES := $(shell nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
MAKEFLAGS += -j$(CORES)

# Phụ thuộc
.PHONY: all c_version cpp_version both debug release profile profile-use pgo arm android wasm
.PHONY: asan ubsan msan tsan clean install uninstall run valgrind lldb perf scan style info help
.PHONY: c_version_debug cpp_version_debug c_version_release cpp_version_release
.PHONY: c_version_profile cpp_version_profile c_version_profile_use cpp_version_profile_use
.PHONY: c_version_asan cpp_version_asan c_version_ubsan cpp_version_ubsan
.PHONY: c_version_msan cpp_version_msan c_version_tsan cpp_version_tsan
.PHONY: c_version_pgo cpp_version_pgo

# Thông báo
.SILENT: clean
