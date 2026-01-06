CXX      := g++
TARGET   := $(or $(EXE),build/maxi-yahtzee)  # Use EXE if defined, else default
SOURCES  := $(wildcard src/*.cpp)
OBJECTS  := $(patsubst src/%.cpp,build/%.o,$(SOURCES))
BUILD_DIR := build

# Default flags
CXXFLAGS := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -std=c++20 -O3 -Isrc -g -fopenmp
LDFLAGS  := -fopenmp

# Toggle debug/sanitizer flags
DEBUG ?= 0
ifeq ($(DEBUG),1)
    CXXFLAGS += -g -O1 -fsanitize=address -fno-omit-frame-pointer
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
