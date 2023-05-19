#ifndef IPC_FUNCTIONS_H
#define IPC_FUNCTIONS_H
#include "pe_common.h"
#include "pe_exchange.h"
// void sigusr1_handler(int, siginfo_t *, void*);
// void sigusr2_handler(int, siginfo_t *, void*);
// void sigint_handler(int, siginfo_t*, void*);

void register_signal(int, void*);

void create_fds(char*, char*, int);
void execute_trader_binary(int, char*);
void write_to_trader(int fd, char* message, int length);
void send_signal_to_trader(pid_t trader_pid);
void write_fill_order(int fd, int order_id, int qty);
void setup_epoll_event_for_traders(struct trader **traders, struct products *exchanging_products, int num_of_traders, 
    int *ex_fds, int *pids, int *tr_fds, int epoll_inst);
#endif