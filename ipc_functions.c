#include "ipc_functions.h"

void register_signal(int signum, void *handler)
{
    struct sigaction sig;
    sig.sa_flags = SA_RESTART | SA_SIGINFO;
    if (signum == SIGUSR1) {
        sig.sa_sigaction = handler;
    } else if (signum == SIGUSR2) {
        sig.sa_sigaction = handler;
    } else if (signum == SIGCHLD) {
        sig.sa_sigaction = handler;
    }
    if (-1 == sigaction(signum, &sig, NULL)) {
        perror("Failed to register signal");
    }
}

void create_fds(char* e_fd, char* t_fd, int i) {
    sprintf(e_fd, FIFO_EXCHANGE, i);
    sprintf(t_fd, FIFO_TRADER, i);
    unlink(e_fd);
    unlink(t_fd);
    if (mkfifo(e_fd, 0666) < 0) {
        perror("failed to create fd");
        exit(EXIT_FAILURE);
    } else {
        printf("%s Created FIFO %s\n",LOG_PREFIX, e_fd);
    }
    if (mkfifo(t_fd, 0666) < 0) {
		perror("failed to create fd");
	} else {
		printf("%s Created FIFO %s\n",LOG_PREFIX, t_fd);
	}
}

void execute_trader_binary(int argc, char* path) {
    char *trader_args[3];
    printf("%s Starting trader %d (%s)\n", LOG_PREFIX, argc, path);
    char id[4];
    sprintf(id, "%d", argc);
    trader_args[0] = path;
    trader_args[1] = id;
    trader_args[2] = NULL;
    if (-1 == execv(path, trader_args))
    {
        exit(EXIT_FAILURE);
    }
    sleep(2);
    exit(0);
}