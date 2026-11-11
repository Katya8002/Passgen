CC     = gcc
CFLAGS = -Iinclude -std=gnu99 -pedantic -Wall -Wextra -Werror -g
LDLIBS = -lm
SRC    = $(wildcard src/*.c)
OBJ    = $(SRC:.c=.o)

.PHONY: all clean

all: passgen

passgen: $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ): $(wildcard include/*.h)

clean:
	rm -f passgen $(OBJ)
