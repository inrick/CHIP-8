CC := gcc
CFLAGS := -g3 -Wall -Wpedantic -std=c99
LDFLAGS := -lglfw -lGL -lGLEW

SRCS := main.c \
        chip8.c
HEADERS := chip8.h

BIN := chip8

OBJS := $(SRCS:.c=.o)

.PHONY: all
all: CFLAGS += -O2
all: $(BIN)

.PHONY: debug
debug: CFLAGS += -O0
debug: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: tags
tags:
	cscope -Rb
	ctags -R .

.PHONY: clean
clean:
	-rm -f *.o tags cscope.out $(BIN)
