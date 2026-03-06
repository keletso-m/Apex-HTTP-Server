CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -I./include
LDFLAGS  :=

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

# ─── Targets ──────────────────────────────────────────────────────────────────

.PHONY: all debug clean dirs run

all: CXXFLAGS += -O2
all: dirs $(BIN)

debug: CXXFLAGS += -g -DDEBUG -O0
debug: dirs $(BIN)

$(BIN): $(OBJS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "\n✅  Build successful → $(BIN)"

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
