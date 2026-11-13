CC     = gcc
CFLAGS = -Iinclude -std=gnu99 -pedantic -Wall -Wextra -Werror -g
LDLIBS = -lm
SRC    = $(wildcard src/*.c)
OBJ    = $(SRC:.c=.o)

CXX         = g++
CXXFLAGS    = -Iinclude -std=c++17 -Wall -Wextra
GTEST_FLAGS = $(shell pkg-config --cflags --libs gtest)
LIB_OBJ     = $(filter-out src/main.o, $(OBJ))
TEST_SRC    = $(wildcard tests/*.cpp)

.PHONY: all test clean

all: passgen

passgen: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ): $(wildcard include/*.h)

test: tests/tests
	./tests/tests

tests/tests: $(TEST_SRC) $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(GTEST_FLAGS)

clean:
	rm -f passgen tests/tests $(OBJ)
