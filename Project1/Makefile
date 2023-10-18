# Parameters
CC = gcc
CFLAGS = -Wall

SRC = src/
INCLUDE = include/
BIN = bin/

# ! CHANGE THIS !
TX_SERIAL_PORT = /dev/ttyS10
RX_SERIAL_PORT = /dev/ttyS11

TX_FILE = penguin.gif
RX_FILE = penguin-received.gif

# Targets
.PHONY: all
all: $(BIN)/main

$(BIN)/main: main.c $(SRC)/*.c
	$(CC) $(CFLAGS) -o $@ $^ -I$(INCLUDE) -lm

.PHONY: run_tx
run_tx: $(BIN)/main
	./$(BIN)/main $(TX_SERIAL_PORT) tx $(TX_FILE)

.PHONY: run_rx
run_rx: $(BIN)/main
	./$(BIN)/main $(RX_SERIAL_PORT) rx $(RX_FILE)

.PHONY: check_files
check_files:
	diff -s $(TX_FILE) $(RX_FILE) || exit 0

.PHONY: clean
clean:
	rm -f $(BIN)/main
	rm -f $(RX_FILE)
