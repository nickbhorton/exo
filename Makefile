CXX = g++
CXXFLAGS = -std=c++20 -Wall -Werror -O1

debug: exo

exo: src/exo.o
	$(CXX) $(CXXFLAGS) -o $@ $^ -llua++5.4

src/exo.o: src/exo.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o
	rm -f exo

.PHONY: debug clean
