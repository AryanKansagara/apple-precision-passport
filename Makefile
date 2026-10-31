CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror
.PHONY: all test demo clean
all: build/precision-passport
build:
	mkdir -p build
build/precision-passport: main.cpp passport.hpp | build
	$(CXX) $(CXXFLAGS) main.cpp -o $@
build/tests: tests.cpp passport.hpp | build
	$(CXX) $(CXXFLAGS) tests.cpp -o $@
test: build/tests build/precision-passport
	./build/tests
	./build/precision-passport demo > /dev/null
demo: build/precision-passport
	./build/precision-passport demo
clean:
	rm -rf build
