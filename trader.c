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
    sig.sa_flags = SA_RESTART|SA_SIGINFO;
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
    int num_of_orders = 12;
    char *message[12] = {
        "BUY 0 GPU 2 50;",
        "CANCEL 0;",
        "BUY 1 GPU 5 55;",
        "BUY 2 GPU 5 10;",
        "BUY 3 GPU 5 5;",
        "BUY 4 GPU 5 2;",
        "SELL 5 GPU 3 70;",
        "SELL 6 GPU 5 65;",
        "AMEND 1 5 45;",
        "SELL 7 GPU 10 80;",
        "CANCEL 1;",
        "SELL 8 GPU 15 50;"

        // "SELL 4 GPU 30 503;",
        // "CANCEL 0;"
        // "BUY 4 GPU 30 501;",
        // "AMEND 4 20 150;"
        // "BUY 4 Router 100;",
        // "AMEND 1 5 45;"
        // "CANCEL 1;",
        // "CANCEL 2;",
        // "BUY 5 GPU 30 502;",
        // "SELL 6 Router 50 100;"
        // "CANCEL 3;"
        
        // "CANCEL 0;"
    };
    // char *message[2] = {
    //     "SELL 0 GPU 99 511;",
    //     "SELL 1 GPU 99 402;"
    // };
    int index = 0;
    while (1)
    {
        sleep(1);
        if (terminate) {
            printf("Terminating\n");
            break;
        }
        char read_buf[128];
        int ret = read(read_fd, read_buf, 128);
        if (-1 == ret)
        {
            perror("failed to read from trader pipe trader");
        }
        // sleep(5);
        printf("%s\n", read_buf);
        // if (strcmp(type, "SELL") == 0) continue;
        // sprintf(write_buf, "%s", msg);
        // sleep(5);
        if (index < num_of_orders) {
            ret = write(write_fd, message[index++], 128);
            if (-1 == ret)
            {
                perror("failed to write to exchange");
                break;
            }
            // if (-1 == kill(getppid(), SIGUSR1)) {
            //     perror("kill error: ");
            // }
            // printf("Written to exchange\n");
            if (kill(getppid(), SIGUSR1) == -1) {
                perror("kill: ");
            }
            sleep(2);
            // printf("written to exchange\n");
        }
        if (index == num_of_orders) {
            // sleep(5);
            break;
        }
        // else {
        //     if (kill(getppid(), SIGUSR1) == -1) {
        //         perror("failed to send: ");
        //     }
            
        // }
    }
    // printf("sending SIGUSR2 from trader_%d\n", trader_id);
    close(write_fd);
    close(read_fd);
    // exit(EXIT_SUCCESS);
}