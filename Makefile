CXX      := g++
TARGET   := build/maxi-yahtzee
SOURCES  := $(wildcard src/*.cpp)
OBJECTS  := $(patsubst src/%.cpp,build/%.o,$(SOURCES))
BUILD_DIR := build

# Default flags
CXXFLAGS := -Wall -Wextra -std=c++20 -O3 -Isrc
LDFLAGS  :=

# Toggle debug/sanitizer flags
DEBUG ?= 0
ifeq ($(DEBUG),1)
    CXXFLAGS += -g -O0 -fsanitize=address -fno-omit-frame-pointer
    LDFLAGS  += -fsanitize=address
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

build/%.o: src/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
