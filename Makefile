CXXFLAGS = -g -Wall -pedantic -std=c++14 -I./include

SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

TARGET = p2p

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

.PHONY: clean
clean:
	-rm p2p
	-rm src/*.o

%.o:%.cpp
	$(CXX) -c $(CXXFLAGS) $^ -o $@
