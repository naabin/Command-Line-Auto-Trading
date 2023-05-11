TARGET=pe_exchange
CC=gcc
CFLAGS=-Wall -Werror -Wvla -O0 -std=c11 -g -fsanitize=address,leak
SRC=pe_exchange.c file_io.c exchange_operation.c ipc_functions.c
OBJ = $(SRC:.c=.o)
LDFLAGS=-lm
# BINARIES=pe_exchange pe_trader

# all: $(BINARIES)
all: $(TARGET)

$(TARGET):$(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

run:
	./$(TARGET) products.txt pe_trader
# .PHONY: clean
clean:
	rm -f *.o *.obj $(TARGET)

