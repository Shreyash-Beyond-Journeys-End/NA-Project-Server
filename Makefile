CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2

bserve: $(wildcard src/*.cpp) $(wildcard src/*.h)
	$(CXX) $(CXXFLAGS) -o bserve $(wildcard src/*.cpp)

test: bserve
	./tests/test.sh

clean:
	rm -f bserve
