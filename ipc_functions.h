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
#endif