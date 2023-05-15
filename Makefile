TARGET1=pe_exchange
TARGET2=pe_trader
CC=gcc
CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak
SRC=pe_exchange.c file_io.c exchange_operation.c ipc_functions.c
SRC2=pe_trader.c
OBJ = $(SRC:.c=.o)
LDFLAGS=-lm
BINARIES=pe_exchange pe_trader

all: $(BINARIES)

$(TARGET2): $(SRC2)
	$(CC) $(CFLAGS) $@.c -o $@

$(TARGET1):$(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

run:
	./$(TARGET1) products.txt pe_trader
# .PHONY: clean
clean:
	rm -f *.o *.obj $(TARGET1)

