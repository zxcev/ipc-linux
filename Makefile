CC = gcc
CFLAGS = -Wall -Wextra -I.

SRCS = $(wildcard *.c)
# object files
OBJS = $(patsubst %.c,dist/%.o,$(SRCS))
# elfs
ELFS = $(patsubst %.c,dist/%,$(SRCS))

# default target
all: $(ELFS)

dist:
	mkdir -p dist

# apply order-only prerequisite to `dist`
dist/%: %.c | dist
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf dist

.PHONY: all clean
