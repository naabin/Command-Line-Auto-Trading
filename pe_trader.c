#include "pe_trader.h"

void signal_handler(int sig, siginfo_t *info, void *context)
{
    return;
}

void handle_error(int ret, char *message)
{
    if (ret < 0)
    {
        perror(message);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Not enough arguments\n");
        return 1;
    }
    // register signal handler
    struct sigaction sig;
    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = signal_handler;
    int signum = sigaction(SIGUSR1, &sig, NULL);
    handle_error(signum, "sigaction failed to send signal");

    // connect to named pipes
    char exchange_fd[20];
    int trader_id = atoi(argv[1]);
    sprintf(exchange_fd, FIFO_EXCHANGE, trader_id);
    char trader_fd[20];
    sprintf(trader_fd, FIFO_TRADER, trader_id);
    int read_fd, write_fd;
    read_fd = open(exchange_fd, O_RDONLY);
    handle_error(read_fd, "failed to open exchange file descriptor");
    write_fd = open(trader_fd, O_WRONLY);
    handle_error(write_fd, "Failed to open trader file descriptor");
    // start order_id from 0 for buy order
    int order_id = 0;
    // event loop:
    while (1)
    {
        // pausing becuse signal is interrupting the following instructions
        pause();
        char buf[128];
        int res = read(read_fd, buf, 128);
        handle_error(res, "Failed to read from exchange pipe");
        char *market = strtok(buf, " ");
        if (strlen(market) != 6)
            continue;
        char *order_type = strtok(NULL, " ");
        // Ignore any buy messages
        if ((strcmp(order_type, "BUY") == 0))
            continue;
        char *product = strtok(NULL, " ");
        // If the product is null the market has just opened, so continue to next message
        if (product == NULL)
            continue;
        int quantity = atoi(strtok(NULL, " "));
        if (quantity >= 1000)
            break;
        int price = atoi(strtok(NULL, ";"));
        char buy_order[128];
        sprintf(buy_order, "BUY %d %s %d %d;", order_id, product, quantity, price);

        // send order
        res = write(write_fd, buy_order, strlen(buy_order));
        handle_error(res, "Failed to write to trader pipe");

        // wait for exchange update (MARKET message)
        while (1)
        {
            // wait for exchange confirmation (ACCEPTED message)
            // Keep sending the signal to exchange until it recieves it
            if (kill(getppid(), SIGUSR1) == -1)
                continue;
            break;
        }
        order_id++;
    }
    close(read_fd);
    close(write_fd);
    return 0;
}