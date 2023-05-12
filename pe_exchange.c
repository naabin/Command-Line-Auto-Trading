/**
 * comp2017 - assignment 3
 * Nabin Karki
 * nkar7555
 */

#include "pe_exchange.h"
#define MAX_CONNECTIONS 5
volatile int EXIT_STATUS = 0;
volatile int TRADER_EXIT_STATUS = -1;
volatile int TRADER_CONNECTION = -1;

// static int compare_fd(const void *a, const void *b) {
// 	struct epoll_event *ea = (struct epoll_event*)a;
// 	struct epoll_event *eb = (struct epoll_event*)b;
// 	return ea->data.fd - eb->data.fd;
// }
void sigusr1_handler(int sig, siginfo_t *info, void *context) {
    TRADER_CONNECTION = info->si_pid;
}
void sigusr2_handler(int sig, siginfo_t *info, void*context) {
    EXIT_STATUS = 1;
    
}
void sigchild_handler(int sig, siginfo_t *info, void*context) {
    TRADER_EXIT_STATUS = info->si_pid;
    sleep(0.1);
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		printf("Not enough arguments\n");
		exit(EXIT_FAILURE);
	}
	struct products *exchanging_products = read_exchaning_prodcuts_from_file(argv[1]);
	print_products(exchanging_products);
	register_signal(SIGUSR1, sigusr1_handler);
	register_signal(SIGUSR2, sigusr2_handler);
	register_signal(SIGCHLD, sigchild_handler);
	register_signal(SIGINT, sigusr2_handler);
	int num_of_traders = argc - 2;
	struct trader **traders = (struct trader **)malloc(sizeof(struct trader *) * num_of_traders);
	struct order_book *book = create_orderbook(10);
	if (argc <= 2) {
		free(traders);
		free_products(exchanging_products);
		exit(EXIT_FAILURE);
	}
	int ex_fds[MAX_CONNECTIONS];
	int tr_fds[MAX_CONNECTIONS];
	pid_t pids[MAX_CONNECTIONS];
	for (int i = 0; i < MAX_CONNECTIONS; i++) {
		ex_fds[i] = -1;
		tr_fds[i] = -1;
	}
	// int epoll_inst = epoll_create(MAX_CONNECTIONS);
	for (int i = 0; i < num_of_traders; i++) {
		char e_fd[20];
		char t_fd[20];
		create_fds(e_fd, t_fd, i);
		pid_t pid = fork();
		if (pid == 0) {
			execute_trader_binary(i, argv[i + 2]);
		}
		if (pid > 0) {
			pids[i] = pid;
			ex_fds[i] = open(e_fd, O_WRONLY);
			printf("%s Connected to %s\n", LOG_PREFIX, e_fd);
			tr_fds[i] = open(t_fd, O_RDONLY);
			printf("%s Connected to %s\n", LOG_PREFIX, t_fd);
		}
	}
	for (int i = 0; i < num_of_traders; i++) {
		// struct epoll_event es = {0};
		// es.events = EPOLLIN | EPOLLET;
		// es.data.ptr = calloc(1, sizeof(struct trader));
		traders[i] = calloc(1, sizeof(struct trader));
		traders[i]->exchange_fd = ex_fds[i];
		traders[i]->exchange_fifo_id = i;
		traders[i]->trader_pid = pids[i];
		// snprintf(traders[i]->ex_fifo_name, 20, FIFO_EXCHANGE, i);
		traders[i]->trader_fd = tr_fds[i];
		traders[i]->trader_fifo_id = i;
		traders[i]->active_status = 1;
		traders[i]->id = i;
		traders[i]->position_price = (int*)calloc(exchanging_products->num_of_products, sizeof(int) * exchanging_products->num_of_products + 1);
		traders[i]->position_qty = (int*)calloc(exchanging_products->num_of_products, sizeof(int) * exchanging_products->num_of_products + 1);
		// snprintf(traders[i]->tr_fifo_name, 20, FIFO_TRADER, i);

		// int ret = epoll_ctl(epoll_inst, EPOLL_CTL_ADD, tr_fds[i], &es);
		// if (ret < 0) {
		// 	perror("epoll_ctl: ");
		// 	return 1;
		// }
	}
	for (int i = 0; i < num_of_traders; i++) {
		char message[128];
		char *string = "MARKET OPEN;";
		sprintf(message, "%s", string);
		if (write(traders[i]->exchange_fd, message, strlen(message)) < 0) {
			perror("Failed to write: ");
		}
		memset(message, 0, 128);
	}
	for (int i = 0; i < num_of_traders; i++) {
		if (-1==kill(traders[i]->trader_pid, SIGUSR1)) {
			perror("Failed to send signal");
		};
	}
	// sleep(2);
	
	//Event loop
	// struct epoll_event events[MAX_CONNECTIONS];
	// qsort(events, ret, sizeof(struct epoll_event), compare_fd);
	int index = 0;
	while (1) {
		// if (TRADER_CONNECTION == -1 && TRADER_EXIT_STATUS == -1) {
		// 	pause();
		// }
		if (index < num_of_traders) {
			// printf("%d\n", EXIT_STATUS);
			struct trader *t = traders[index];
			if (TRADER_EXIT_STATUS == t->trader_pid) {
				printf("%s Trader %d disconnected\n", LOG_PREFIX, t->id);
				break;
			}
			else if (TRADER_CONNECTION == -1) {
				pause();
			// continue;
			}
			else if (t != NULL && t->active_status) {
				// int status;
				// pid_t result = waitpid(-1, &status, WNOHANG);
				// if (result == -1) {
				// 	perror("waitpid: ");
				// 	// break;
				// }
				// else if (result == 0) {
					char buffer[128];
					int r = read(t->trader_fd, buffer, 128);
					if (r < 0) {
						perror("Failed to read: ");
						// t->active_status = 0;
						break;
					}
					if (TRADER_EXIT_STATUS == t->trader_pid) {
						// t->active_status = 0;
						index += 1;
						continue;
					}
					// printf("Trader exit status %d\n", TRADER_EXIT_STATUS);
					if (strlen(buffer) <= 0){
						// printf("No more to read\n");
						continue;
					}
					for (int i = 0; i < strlen(buffer); i++) {
						if (buffer[i] == ';') {
							buffer[i] = '\0';
							break;
						}
					}
					printf("%s [T%d] Parsing command: <%s>\n", LOG_PREFIX, t->trader_fifo_id, buffer);
					char * order_type = strtok(buffer, " ");
					int order_id = atoi(strtok(NULL, " "));
					char * product_name = strtok(NULL, " ");
					int quantity = atoi(strtok(NULL, " "));
					int price = atoi(strtok(NULL, ";"));
					int trader_id = t->trader_fifo_id;
					enqueue_order(book, order_type, order_id, product_name, quantity, price, trader_id);
					print_orderbook(book, exchanging_products);
					print_position(exchanging_products, traders, num_of_traders);
					// free(new_order);
					char m[128];
					sprintf(m, "ACCEPTED %d;", order_id);
					if (write(t->exchange_fd, m, 128) < 1) {
						perror("writing error: ");
					}
					if (-1 == kill(t->trader_pid, SIGUSR1)) {
						perror("Failed to kill: ");
					}
					sleep(0.1);
					memset(buffer, 0, 128);
					char msg[128];
					char *o_type = strcmp(order_type, "BUY") == 0 ? "SELL" : "BUY";
					sprintf(msg, "MARKET %s %s %d %d;", o_type, product_name, quantity, price);
					// printf("Broadcasting message\n");
					for (int i = 0; i < num_of_traders; i++) {
						if (traders[i]->id != t->id) {
							if (write(traders[i]->exchange_fd, msg, strlen(msg)) < 0) {
								perror("write: ");
							}
							if (-1 == kill(traders[i]->trader_pid, SIGUSR1)) {
								perror("kill: ");
							}
							sleep(0.1);
						}	
					}
				// }
				// else if (WIFEXITED(status)) {
					// printf("%s Trader %d disconnected\n", LOG_PREFIX, t->id);
					// char e_fifo[20], t_fifo[20];
					// sprintf(e_fifo, FIFO_EXCHANGE, t->id);
					// sprintf(t_fifo, FIFO_TRADER, t->id);
					// unlink(e_fifo);
					// unlink(t_fifo);
					// if (index < num_of_traders) continue;
					// else break;
			}
		} else if (index == num_of_traders) {
			int count = 0;
			for (int i = 0; i < num_of_traders; i++) {
				if (traders[i]->trader_pid == TRADER_EXIT_STATUS) {
						traders[i]->active_status = 0;
						printf("%s Trader %d disconnected\n", LOG_PREFIX, traders[i]->id);
				}
				if (traders[i]->active_status == 0) {
					count++;
				} 
			}
			if (count == num_of_traders) {
				break;
			}
		}
	}
	printf("%s Trading completed\n", LOG_PREFIX);
	printf("%s Exchange fees collected: $%d\n", LOG_PREFIX, 0);
	for (int i = 0; i < num_of_traders; i++)
	{
		char e_fifo[20], t_fifo[20];
		sprintf(e_fifo, FIFO_EXCHANGE, traders[i]->id);
		sprintf(t_fifo, FIFO_TRADER, traders[i]->id);
		unlink(e_fifo);
		unlink(t_fifo);
		free(traders[i]->position_qty);
		free(traders[i]->position_price);
		free(traders[i]);
	}
	free(traders);
	free_products(exchanging_products);
	free_orderbook(book);
	return 0;
}
