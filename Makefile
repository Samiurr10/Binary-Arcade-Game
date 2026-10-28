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
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR)/binary-arcade: $(LIB_OBJS) $(BUILD_DIR)/src/main.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/test-runner: $(LIB_OBJS) $(BUILD_DIR)/tests/test_main.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/benchmark: $(LIB_OBJS) $(BUILD_DIR)/tools/benchmark.o
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD_DIR)/esp32-tests: tests/test_esp32.cpp firmware/esp32/include/arcade.hpp firmware/esp32/include/protocol.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -Ifirmware/esp32/include $< -o $@

test: $(BUILD_DIR)/test-runner $(BUILD_DIR)/esp32-tests
	./$(BUILD_DIR)/test-runner
	./$(BUILD_DIR)/esp32-tests
	python3 firmware/esp32/tools/embed_web.py --check
	python3 tests/test_simantic_image.py

benchmark: $(BUILD_DIR)/benchmark
	./$(BUILD_DIR)/benchmark

run: $(BUILD_DIR)/binary-arcade
	./$(BUILD_DIR)/binary-arcade

clean:
	rm -rf $(BUILD_DIR)

-include $(LIB_OBJS:.o=.d) $(BUILD_DIR)/src/main.d $(BUILD_DIR)/tests/test_main.d $(BUILD_DIR)/tools/benchmark.d

$(BUILD_DIR)/esp32-replay: tools/esp32_replay.cpp firmware/esp32/include/arcade.hpp firmware/esp32/include/protocol.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: replay
replay: $(BUILD_DIR)/esp32-replay
	./$(BUILD_DIR)/esp32-replay > $(BUILD_DIR)/preview.json
