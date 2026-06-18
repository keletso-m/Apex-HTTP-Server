CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -I./include
LDFLAGS  := -pthread  # fixed: was declared twice

# Directories
SRC_DIRS := src/core src/http src/handlers src/utils
BUILD    := build
BIN      := bin/apex-server

# Collect all .cpp files
SRCS := src/main.cpp \
        $(wildcard src/core/*.cpp) \
        $(wildcard src/http/*.cpp) \
        $(wildcard src/handlers/*.cpp) \
        $(wildcard src/utils/*.cpp)

OBJS := $(patsubst %.cpp, $(BUILD)/%.o, $(SRCS))

#  Targets and rules

.PHONY: all debug clean dirs run benchmark profile

all: CXXFLAGS += -O2
all: dirs $(BIN)

debug: CXXFLAGS += -g -DDEBUG -O0
debug: dirs $(BIN)

$(BIN): $(OBJS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "\n  Build successful → $(BIN)"

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

dirs:
	@mkdir -p $(BUILD) bin logs www

run: all
	./$(BIN)

clean:
	rm -rf $(BUILD) bin
	@echo "Cleaned."

# benchmark 
# sudo apt install wrk
benchmark: all
	@echo "\n  Starting server in background..."
	@./$(BIN) > /dev/null 2>&1 &
	@sleep 1
	@echo "\n Benchmark: 2 threads, 100 connections, 10 seconds"
	@wrk -t2 -c100 -d10s --latency http://localhost:8080/ || true
	@echo "\n  Benchmark: 2 threads, 400 connections, 10 seconds"
	@wrk -t2 -c400 -d10s http://localhost:8080/ || true
	@echo "\n  Stopping server..."
	@pkill apex-server || true
	@echo "Done."

#Profiling 
# sudo apt install linux-perf
profile: debug
	@echo "\n🔍  Starting server under perf..."
	@echo "    Run your benchmark in another terminal, then Ctrl+C here."
	perf stat ./$(BIN)