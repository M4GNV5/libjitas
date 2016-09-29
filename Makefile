CC = gcc
CFLAGS = -Wall -O2 -D_GNU_SOURCE --std=c99 -g

BIN = bin

OBJECTS += $(BIN)/error.o
OBJECTS += $(BIN)/regs.o
OBJECTS += $(BIN)/ops.o
OBJECTS += $(BIN)/opencode.o
OBJECTS += $(BIN)/parse.o
OBJECTS += $(BIN)/jitas.o

all: $(BIN) $(OBJECTS)
	ar rcs $(BIN)/libjitas.a $(OBJECTS)

clean:
	if [ -d $(BIN) ]; then rm -r $(BIN); fi

$(BIN):
	mkdir $(BIN)

$(BIN)/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@
