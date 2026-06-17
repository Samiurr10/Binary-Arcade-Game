CXX ?= c++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Werror -pedantic
CPPFLAGS ?= -Iinclude
BUILD_DIR := build
LIB_SRCS := src/framebuffer.cpp src/game.cpp src/bus.cpp src/flash_iap.cpp src/system.cpp
LIB_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(LIB_SRCS))

.PHONY: all test benchmark clean run

all: $(BUILD_DIR)/binary-arcade

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)/src $(BUILD_DIR)/tests $(BUILD_DIR)/tools

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/binary-arcade: $(LIB_OBJS) $(BUILD_DIR)/src/main.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/test-runner: $(LIB_OBJS) $(BUILD_DIR)/tests/test_main.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/benchmark: $(LIB_OBJS) $(BUILD_DIR)/tools/benchmark.o
	$(CXX) $(CXXFLAGS) $^ -o $@

test: $(BUILD_DIR)/test-runner
	./$(BUILD_DIR)/test-runner

benchmark: $(BUILD_DIR)/benchmark
	./$(BUILD_DIR)/benchmark

run: $(BUILD_DIR)/binary-arcade
	./$(BUILD_DIR)/binary-arcade

clean:
	rm -rf $(BUILD_DIR)
