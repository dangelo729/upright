# upright — extras/host/Makefile  (plan §15)
# the host rig: everything compiles here with clang and nothing else.
#
#   make test     build and run every test in tests/
#   make tool     build/uptool, the one cli
#   make corpus   the library's byte baseline: uptool corpus into build/corpus
#   make clean
#
# gnu make 3.81 compatible (the macos default).

CXX      := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -Werror -O2 -fno-math-errno -I../../src -I.

BUILD := build

CORE_SRC := $(wildcard ../../src/core/*.cpp)
WRAP_SRC := $(wildcard ../../src/*.cpp)
TEST_SRC := $(wildcard tests/*.cpp)
HOST_SRC := up_wav.cpp up_png.cpp

CORE_OBJ := $(patsubst ../../src/core/%.cpp,$(BUILD)/core_%.o,$(CORE_SRC))
WRAP_OBJ := $(patsubst ../../src/%.cpp,$(BUILD)/wrap_%.o,$(WRAP_SRC))
TEST_OBJ := $(patsubst tests/%.cpp,$(BUILD)/test_%.o,$(TEST_SRC))
HOST_OBJ := $(patsubst %.cpp,$(BUILD)/%.o,$(HOST_SRC))

.PHONY: all test tool corpus clean

all: $(BUILD)/run_tests $(BUILD)/uptool

test: $(BUILD)/run_tests
	./$(BUILD)/run_tests

tool: $(BUILD)/uptool

corpus: $(BUILD)/uptool
	./$(BUILD)/uptool corpus $(BUILD)/corpus

clean:
	rm -rf $(BUILD)

$(BUILD)/run_tests: $(TEST_OBJ) $(CORE_OBJ) $(WRAP_OBJ) $(HOST_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD)/uptool: $(BUILD)/uptool.o $(CORE_OBJ) $(HOST_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

# -MMD -MP: header dependencies in build/*.d, included below
$(BUILD)/core_%.o: ../../src/core/%.cpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# the teensy wrappers (stage 3): empty translation units off the board
$(BUILD)/wrap_%.o: ../../src/%.cpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# the tests read src/core's sources (the libm and heap guard) and the reference tables
$(BUILD)/test_%.o: tests/%.cpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -DUP_CORE_DIR=\"$(abspath ../../src/core)\" -DUP_REF_DIR=\"$(abspath ../reference)\" -MMD -MP -c $< -o $@

$(BUILD)/%.o: %.cpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -DUP_REF_DIR=\"$(abspath ../reference)\" -MMD -MP -c $< -o $@

-include $(wildcard $(BUILD)/*.d)
