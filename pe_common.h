#ifndef PE_COMMON_H
#define PE_COMMON_H

#define _POSIX_SOURCE
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/epoll.h>

#define FIFO_EXCHANGE "/tmp/pe_exchange_%d"
#define FIFO_TRADER "/tmp/pe_trader_%d"
#define FEE_PERCENTAGE 1

struct order {
    int trader_id;
    int order_id;
    char *order_type;
    char *product_name;
    int num_of_orders;
    int quantity;
    int price;
};

struct order_book {
    char *product_name;
    int buy_level;
    int sell_level;
    struct order *orders;
    int size;
    int capaity;
    int (*compare_orders)(struct order*, struct order*);
};


struct products
{
    int num_of_products;
    char **items;
};

struct trader {
    int id;
    pid_t trader_pid;
    int active_status;
    int exchange_fd;
    int trader_fd;
    int trader_fifo_id;
    int exchange_fifo_id;
    char ex_fifo_name[20];
    char tr_fifo_name[20];
    int *position_qty;
    int *position_price;
};
//file I/O
void print_orderbook(struct order_book *book, struct products*);
struct order_book* create_orderbook(int order_size);
void free_orderbook(struct order_book* book);
void enqueue_order(struct order_book *book, char *, int, char*, int, int, int);
void print_position(struct products *, struct trader **, int);
#endif
