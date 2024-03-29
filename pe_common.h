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
#include <time.h>
#include <sys/epoll.h>
#include <math.h>

#define FIFO_EXCHANGE "/tmp/pe_exchange_%d"
#define FIFO_TRADER "/tmp/pe_trader_%d"
#define FEE_PERCENTAGE 1

typedef void (*write_fill)(int fd, int order_id, int qty);
typedef void (*send_sig)(pid_t pid);

struct order
{
    int order_id;
    char *order_type;
    char *product_name;
    int num_of_orders;
    long quantity;
    long price;
    // Temporary solution to not show the fulfilled order to stdout
    int fulfilled;
    int is_same;
    struct trader *trader;
    struct order **same_orders;
};

struct order_book
{
    char *product_name;
    struct order **orders;
    int size;
    int capaity;
    int (*compare_orders)(struct order *, struct order *);
};

struct product
{
    int buy_level;
    int sell_level;
    char *product_name;
};

struct products
{
    int num_of_products;
    struct product **itms;
};

struct trader
{
    int id;
    int current_order_id;
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
typedef void (*send_invalid)(struct trader *t, char *invalid_message);

long extract_int_value(char*invalid_message, struct trader *t, send_invalid, int is_id);
char *extract_string_value(char*invalid_message, struct trader *t, send_invalid, int is_id);

void print_orderbook(struct order_book *book, struct products *);
struct order_book *create_orderbook(int order_size);
void free_orderbook(struct order_book *book);
struct order *enqueue_order(struct order_book *book, char *, int, char *, long, long, struct trader *t);
void print_position(struct products *, struct trader **, int);
int cancel_order(struct order_book *book, int order_id, struct trader *t, struct products *available_products, struct trader **traders, int num_of_traders);
int update_order(struct order_book *book, int order_id, long new_quanity, long new_price, struct trader *t, struct trader **traders, int num_of_traders, struct products *available_products);
int check_if_product_exist(struct products *available_products, char *new_product_name);
void increment_level(struct products *available_products, char *order_type, char *product_name);
void process_sell_order(struct order *new_order, struct order_book *book, struct trader *t, struct products *available_products, write_fill, send_sig, int *fees);
void process_buy_order(struct order *new_order, struct order_book *book, struct trader *t, struct products *available_products, write_fill fill_message, send_sig signal_traders, int *fees);
#endif
