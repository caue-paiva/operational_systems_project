#variaveis
CXX = g++
CXXFLAGS = -std=c++17 
SOURCES = $(wildcard *.cpp)
EXECUTABLE = program

all: $(EXECUTABLE) #compilar

$(EXECUTABLE): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(EXECUTABLE) $(SOURCES)

run: all #executa
	./$(EXECUTABLE)

clean: #limpa
	rm -f *.out *.o
