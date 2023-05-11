#include "pe_common.h"

volatile int terminate = 0;
void signal_handler(int sig, siginfo_t *info, void *context)
{
}
void handler(int signum) {
    terminate = 1;
}


int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Not enough arguments\n");
        kill(getppid(), SIGINT);
        exit(EXIT_FAILURE);
    }
    struct sigaction sig;
    sig.sa_flags = SA_SIGINFO;
    sig.sa_sigaction = signal_handler;
    int signum = sigaction(SIGUSR1, &sig, NULL);
    signal(SIGCHLD, handler);
    if (signum < -1)
    {
        perror("failed to send signal");
        exit(EXIT_FAILURE);
    }
    // connect to named pipes
    char exchange_fd[20];
    int trader_id = atoi(argv[1]);
    int read_fd, write_fd;
    sprintf(exchange_fd, FIFO_EXCHANGE, trader_id);
    char trader_fd[20];
    sprintf(trader_fd, FIFO_TRADER, trader_id);

    read_fd = open(exchange_fd, O_RDONLY);
    if (read_fd < 0)
    {
        perror("failed to open exchange fd");
    }
    write_fd = open(trader_fd, O_WRONLY);
    if (write_fd < 0)
    {
        perror("failed to open trader fd");
    }
    // printf("%d %d\n", read_fd, write_fd);
    int num_of_orders = 2;
    // char *message[4] = {
    //     "BUY 0 GPU 30 500",
    //     "BUY 1 GPU 30 501",
    //     "BUY 2 GPU 30 501",
    //     "BUY 3 GPU 30 502"
    // };
    char *message[2] = {
        "SELL 0 GPU 99 511;",
        "SELL 1 GPU 99 402;"
    };
    int index = 0;
    while (index < num_of_orders)
    {
        if (terminate) {
            printf("Terminating\n");
            break;
        }
        char read_buf[128];
        int ret = read(read_fd, read_buf, 128);
        printf("%s\n", read_buf);
        if (-1 == ret)
        {
            perror("failed to read from trader pipe trader");
            break;
        }
        // printf("%s\n", read_buf);
        // sprintf(write_buf, "%s", msg);
        ret = write(write_fd, message[index++], 128);
        if (-1 == ret)
        {
            perror("failed to write to exchange");
            break;
        }
        if (-1 == kill(getppid(), SIGUSR1)) {
            perror("kill error: ");
        }
        printf("Written to exchange\n");
        // while (1)
        // {
        //     if (kill(getppid(), SIGUSR1) == -1)
        //         continue;
        //     // char b[128];
        //     // ret = read(read_fd, b, 128);
        //     // printf("%s\n", b);
        //     // memset(b, 0, 128);
        //     break;
        // }
        pause();
    }
    // printf("sending SIGUSR2 from trader_%d\n", trader_id);
    if (kill(getppid(), SIGUSR2) == -1) {
        perror("failed to send: ");
    }
    close(write_fd);
    close(read_fd);
    // exit(EXIT_SUCCESS);
}