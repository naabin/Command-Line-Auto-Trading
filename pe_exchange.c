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

int TOTAL_FEES = 0;

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
	int epoll_inst = epoll_create(MAX_CONNECTIONS);
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
			tr_fds[i] = open(t_fd, O_RDONLY | O_NONBLOCK);
			printf("%s Connected to %s\n", LOG_PREFIX, t_fd);
		}
	}
	for (int i = 0; i < num_of_traders; i++) {
		struct epoll_event es = {0};
		es.events = EPOLLIN | EPOLLET;
		es.data.ptr = calloc(1, sizeof(struct trader));
		traders[i] = es.data.ptr;
		traders[i]->exchange_fd = ex_fds[i];
		traders[i]->exchange_fifo_id = i;
		traders[i]->trader_pid = pids[i];
		// sprintf(traders[i]->ex_fifo_name, FIFO_EXCHANGE, (short)i);
		traders[i]->trader_fd = tr_fds[i];
		traders[i]->trader_fifo_id = i;
		traders[i]->active_status = 1;
		traders[i]->id = i;
		traders[i]->current_order_id = 0;
		traders[i]->position_price = (int*)calloc(exchanging_products->num_of_products, sizeof(int) * exchanging_products->num_of_products + 1);
		traders[i]->position_qty = (int*)calloc(exchanging_products->num_of_products, sizeof(int) * exchanging_products->num_of_products + 1);
		// sprintf(traders[i]->tr_fifo_name, FIFO_TRADER, (short)i);
		int ret = epoll_ctl(epoll_inst, EPOLL_CTL_ADD, tr_fds[i], &es);
		if (ret < 0) {
			perror("epoll_ctl: ");
			return 1;
		}
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
	sleep(0.1);
	
	//Event loop
	while (1) {
		// pause();
		struct epoll_event events[MAX_CONNECTIONS];	
		int ret = epoll_wait(epoll_inst, events, MAX_CONNECTIONS, -1);
		if (ret < -1) {
			perror("Ret: ");
			break;
		}
		for (int i = 0; i < ret; i++) {
			struct trader* t = events[i].data.ptr;
			char buffer[128];
			ssize_t bytes_read = read(t->trader_fd, buffer, 128);
			if (bytes_read > 0 && t->active_status) {
				for (int i = 0; i < bytes_read; i++) {
					if (buffer[i] == ';') {
						buffer[i] = '\0';
						break;
					}
				}
				printf("%s [T%d] Parsing command: <%s>\n", LOG_PREFIX, t->trader_fifo_id, buffer);
				//check if the product exist
				// check the type of the order
				// check the price of the quantity and price
				char *invalid_message = malloc(sizeof(char)* INPUT_LENGTH);
				sprintf(invalid_message, "INVALID;");
				char * order_type = strtok(buffer, " ");
				//TODO: handle error for all inputs with strtok
				if (strcmp(CANCEL, order_type) == 0) {
					//validate the id and process the cancel
					int order_id = atoi(strtok(NULL, ";"));
					int cancelled = cancel_order(book, order_id, t, exchanging_products);
					// remove the order from the orderbook
					if (cancelled) {
						char *msg = malloc(sizeof(char) * INPUT_LENGTH);
						sprintf(msg, "CANCELLED %d;", order_id);
						write_to_trader(t->exchange_fd, msg, strlen(msg));
						send_signal_to_trader(t->trader_pid);
						free(invalid_message);
						free(msg);
					} else {
						//send invalid message
						write_to_trader(t->exchange_fd, invalid_message, strlen(invalid_message));
						send_signal_to_trader(t->trader_pid);
						free(invalid_message);
						continue;
					}

				} else if (strcmp(AMEND, order_type) == 0) {
					// validate the id and ammend the order by given price and quantity
					int order_id = atoi(strtok(NULL, " "));
					int new_qty = atoi(strtok(NULL, " "));
					int new_price = atoi(strtok(NULL, ";"));
					int updated = update_order(book, order_id, new_qty, new_price, t);
					if (updated) {
						char* msg = malloc(sizeof(char) * INPUT_LENGTH);
						sprintf(msg, "UPDATED %d;", order_id);
						write_to_trader(t->exchange_fd, msg, strlen(msg));
						send_signal_to_trader(t->trader_pid);
						free(invalid_message);
						free(msg);
					} else {
						// send invalid
						write_to_trader(t->exchange_fd, invalid_message, strlen(invalid_message));
						send_signal_to_trader(t->trader_pid);
						free(invalid_message);
						continue;

					}
				} else if (strcmp(BUY, order_type) == 0 || strcmp(SELL, order_type) == 0) {
					// validate the id and product
					char *id = strtok(NULL, " ");
					if (id == NULL) {
						write_to_trader(t->exchange_fd, invalid_message, strlen(invalid_message));
						send_signal_to_trader(t->trader_pid);
						free(invalid_message);
						continue;
					}
					int order_id = atoi(id);
					if (order_id != t->current_order_id) {
						// send invalid
						write_to_trader(t->exchange_fd, invalid_message, strlen(invalid_message));
						send_signal_to_trader(t->trader_pid);
						free(invalid_message);
						continue;
					}
					char * product_name = strtok(NULL, " ");
					int product_exist = check_if_product_exist(exchanging_products, product_name);
					if (!product_exist) {
						// send invalid
						write_to_trader(t->exchange_fd, invalid_message, strlen(invalid_message));
						send_signal_to_trader(t->trader_pid);
						free(invalid_message);
						continue;
					}
					int quantity = atoi(strtok(NULL, " "));
					if (quantity < 1 || quantity > 999999) {
						// send invalid
						write_to_trader(t->exchange_fd, invalid_message, strlen(invalid_message));
						send_signal_to_trader(t->trader_pid);
						free(invalid_message);
						continue;
					}

					int price = atoi(strtok(NULL, ";"));
					if (price < 1 || price > 999999) {
						write_to_trader(t->exchange_fd, invalid_message, strlen(invalid_message));
						send_signal_to_trader(t->trader_pid);
						free(invalid_message);
						continue;
					}
					int trader_id = t->id;
					//add order to the order book
					struct order *new_order = enqueue_order(book, order_type, order_id, product_name, quantity, price, trader_id, t);
					//incerement the level of the product
					if (new_order->num_of_orders == 1) {
						increment_level(exchanging_products, order_type, product_name);
					}
					// send the accept message
					char *accept_message = malloc(sizeof(char) * INPUT_LENGTH);
					sprintf(accept_message, "ACCEPTED %d;", order_id);
					write_to_trader(t->exchange_fd, accept_message, strlen(accept_message));
					send_signal_to_trader(t->trader_pid);
					free(accept_message);
					// broadcast the message to other traders
					char *market_message = malloc(sizeof(char) * INPUT_LENGTH);
					sprintf(market_message, "MARKET %s %s %d %d;", order_type, product_name, quantity, price);
					for (int i = 0; i < num_of_traders; i++) {
						if (traders[i]->id != t->id && traders[i]->active_status) {
							write_to_trader(traders[i]->exchange_fd, market_message, strlen(market_message));
							send_signal_to_trader(traders[i]->trader_pid);
						}
					}
					free(market_message);
					free(invalid_message);
					// process the given order
					if (strcmp(order_type, SELL) == 0) {
						process_sell_order
						(new_order, book, t, exchanging_products, write_fill_order, send_signal_to_trader, &TOTAL_FEES);
					} else if (strcmp(order_type, BUY) == 0) {
						// printf("%s buy order processing\n", LOG_PREFIX);
						process_buy_order(new_order, book, t, exchanging_products, write_fill_order, send_signal_to_trader, &TOTAL_FEES);
					}
					// increment the trader's current order id
					t->current_order_id++;
				} else {
					// Provided command is invalid. 
					write_to_trader(t->trader_fd, invalid_message, strlen(invalid_message));
					send_signal_to_trader(t->trader_pid);
					free(invalid_message);
				}
				print_orderbook(book, exchanging_products);
				print_position(exchanging_products, traders, num_of_traders);
			}
			else if (bytes_read == 0) {
				// end of file read reached
				// do something
			}
		}
		int active = 0;
		for (int j = 0; j < num_of_traders; j++) {
			if (TRADER_EXIT_STATUS == traders[j]->trader_pid) {
				printf("%s Trader %d disconnected\n", LOG_PREFIX, traders[j]->id);
				traders[j]->active_status = 0;
			}
			if (traders[j]->active_status) {
				active++;
			}
		}
		TRADER_EXIT_STATUS = -1;
		if (active == 0) {
			break;
		}
	}
	printf("%s Trading completed\n", LOG_PREFIX);
	printf("%s Exchange fees collected: $%d\n", LOG_PREFIX, TOTAL_FEES);
	for (int i = 0; i < num_of_traders; i++)
	{
		char e_fifo[20], t_fifo[20];
		close(traders[i]->exchange_fd);
		close(traders[i]->trader_fd);
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
