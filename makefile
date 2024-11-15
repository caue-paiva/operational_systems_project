# Variables
CXX = g++
CXXFLAGS = -std=c++17 
SOURCES = $(wildcard *.cpp)
EXECUTABLE = program

# Default target: Compile everything
all: $(EXECUTABLE)

# Compile the program
$(EXECUTABLE): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(EXECUTABLE) $(SOURCES)

# Run the program
run: all
	./$(EXECUTABLE)

# Clean up build files
clean:
	rm -f *.out *.o
