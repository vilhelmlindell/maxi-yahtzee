CXX      := g++
CXXFLAGS := -Wall -Wextra -std=c++23 -g -O3 -fconcepts-diagnostics-depth=2

TARGET   := maxi-yahtzee
SOURCES  := $(wildcard *.cpp)
OBJECTS  := $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
