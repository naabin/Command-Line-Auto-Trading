/**
 * comp2017 - assignment 3
 * Nabin Karki
 * nkar7555
 */

#include "pe_exchange.h"
#define MAX_CONNECTIONS 20
volatile int EXIT_STATUS = 0;
volatile int TRADER_EXIT_STATUS = -1;
volatile int TRADER_CONNECTION = -1;

void sigusr1_handler(int sig, siginfo_t *info, void *context)
{
	TRADER_CONNECTION = info->si_pid;
}
void sigusr2_handler(int sig, siginfo_t *info, void *context)
{
	EXIT_STATUS = 1;
}
void sigchild_handler(int sig, siginfo_t *info, void *context)
{
	TRADER_EXIT_STATUS = info->si_pid;
	sleep(0.1);
}

void send_invalid_message_to_current_trader(struct trader *t, char *invalid_message)
{
	write_to_trader(t->exchange_fd, invalid_message, strlen(invalid_message));
	send_signal_to_trader(t->trader_pid);
	free(invalid_message);
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
	struct order_book *book = create_orderbook(1);
	if (argc <= 2)
	{
		free(traders);
		free_products(exchanging_products);
		free_orderbook(book);
		exit(EXIT_FAILURE);
	}
	int ex_fds[MAX_CONNECTIONS];
	int tr_fds[MAX_CONNECTIONS];
	pid_t pids[MAX_CONNECTIONS];
	for (int i = 0; i < MAX_CONNECTIONS; i++)
	{
		ex_fds[i] = -1;
		tr_fds[i] = -1;
	}
	int epoll_inst = epoll_create(MAX_CONNECTIONS);
	for (int i = 0; i < num_of_traders; i++)
	{
		char e_fd[20];
		char t_fd[20];
		create_fds(e_fd, t_fd, i);
		pid_t pid = fork();
		if (pid == 0)
		{
			execute_trader_binary(i, argv[i + 2]);
		}
		if (pid > 0)
		{
			pids[i] = pid;
			ex_fds[i] = open(e_fd, O_WRONLY);
			printf("%s Connected to %s\n", LOG_PREFIX, e_fd);
			tr_fds[i] = open(t_fd, O_RDONLY | O_NONBLOCK);
			printf("%s Connected to %s\n", LOG_PREFIX, t_fd);
		}
	}
	setup_epoll_event_for_traders(traders, exchanging_products, num_of_traders, ex_fds, pids, tr_fds, epoll_inst);
	for (int i = 0; i < num_of_traders; i++)
	{
		char message[128];
		char *string = "MARKET OPEN;";
		sprintf(message, "%s", string);
		write_to_trader(traders[i]->exchange_fd, message, strlen(message));
		memset(message, 0, 128);
	}
	for (int i = 0; i < num_of_traders; i++)
	{
		send_signal_to_trader(traders[i]->trader_pid);
	}
	sleep(0.1);

	// Event loop
	while (1)
	{
		// pause();
		struct epoll_event events[MAX_CONNECTIONS];
		int ret = epoll_wait(epoll_inst, events, MAX_CONNECTIONS, -1);
		if (ret < -1)
		{
			perror("Ret: ");
			break;
		}
		for (int i = 0; i < ret; i++)
		{
			struct trader *t = events[i].data.ptr;
			char buffer[128];
			ssize_t bytes_read = read(t->trader_fd, buffer, 128);
			if (bytes_read > 0 && t->active_status)
			{
				for (int i = 0; i < bytes_read; i++)
				{
					if (buffer[i] == ';')
					{
						buffer[i] = '\0';
						break;
					}
				}
				printf("%s [T%d] Parsing command: <%s>\n", LOG_PREFIX, t->trader_fifo_id, buffer);
				// check if the product exist
				//  check the type of the order
				//  check the price of the quantity and price
				char *invalid_message = malloc(sizeof(char) * INPUT_LENGTH);
				sprintf(invalid_message, "INVALID;");
				char *order_type = strtok(buffer, " ");
				if (order_type == NULL)
				{
					send_invalid_message_to_current_trader(t, invalid_message);
					continue;
				}
				// TODO: handle error for all inputs with strtok
				if (strcmp(CANCEL, order_type) == 0)
				{
					// validate the id and process the cancel
					// char *id = strtok(NULL, ";");
					// if (id == NULL)
					// {
					// 	send_invalid_message_to_current_trader(t, invalid_message);
					// 	continue;
					// }
					// int order_id = atoi(id);
					int order_id = extract_int_value(invalid_message, t, send_invalid_message_to_current_trader, 1);
					if (order_id < 0) continue;
					int cancelled = cancel_order(book, order_id, t, exchanging_products, traders, num_of_traders);
					// remove the order from the orderbook
					if (cancelled)
					{
						char *msg = malloc(sizeof(char) * INPUT_LENGTH);
						sprintf(msg, "CANCELLED %d;", order_id);
						write_to_trader(t->exchange_fd, msg, strlen(msg));
						send_signal_to_trader(t->trader_pid);
						free(invalid_message);
						free(msg);
					}
					else
					{
						// send invalid message
						send_invalid_message_to_current_trader(t, invalid_message);
						continue;
					}
				}
				else if (strcmp(AMEND, order_type) == 0)
				{
					// validate the id and ammend the order by given price and quantity
					// char *id = strtok(NULL, " ");
					// if (id == NULL)
					// {
					// 	send_invalid_message_to_current_trader(t, invalid_message);
					// }
					int order_id = extract_int_value(invalid_message, t, send_invalid_message_to_current_trader, 1);
					// int order_id = atoi(id);
					// char *qty = strtok(NULL, " ");
					// if (qty == NULL)
					// {
					// 	send_invalid_message_to_current_trader(t, invalid_message);
					// 	continue;
					// }

					long new_qty = extract_int_value(invalid_message, t, send_invalid_message_to_current_trader, 0);
					// char *price = strtok(NULL, ";");
					// if (price == NULL)
					// {
					// 	send_invalid_message_to_current_trader(t, invalid_message);
					// }

					long new_price = extract_int_value(invalid_message, t, send_invalid_message_to_current_trader, 0);
					int updated = update_order(book, order_id, new_qty, new_price, t, traders, num_of_traders, exchanging_products);
					if (updated)
					{
						char *msg = malloc(sizeof(char) * INPUT_LENGTH);
						sprintf(msg, "AMENDED %d;", order_id);
						write_to_trader(t->exchange_fd, msg, strlen(msg));
						send_signal_to_trader(t->trader_pid);
						free(invalid_message);
						free(msg);
					}
					else
					{
						// send invalid
						send_invalid_message_to_current_trader(t, invalid_message);
						continue;
					}
				}
				else if (strcmp(BUY, order_type) == 0 || strcmp(SELL, order_type) == 0)
				{
					// validate the id and product
					// char *id = strtok(NULL, " ");
					// if (id == NULL)
					// {
					// 	send_invalid_message_to_current_trader(t, invalid_message);
					// 	continue;
					// }
					int order_id = extract_int_value(invalid_message, t, send_invalid_message_to_current_trader, 1);
					if (order_id != t->current_order_id)
					{
						// send invalid
						write_to_trader(t->exchange_fd, invalid_message, strlen(invalid_message));
						send_signal_to_trader(t->trader_pid);
						free(invalid_message);
						continue;
					}
					char *product_name = strtok(NULL, " ");
					if (product_name == NULL)
					{
						send_invalid_message_to_current_trader(t, invalid_message);
					}
					int product_exist = check_if_product_exist(exchanging_products, product_name);
					if (!product_exist)
					{
						// send invalid
						send_invalid_message_to_current_trader(t, invalid_message);
						continue;
					}
					char *qty = strtok(NULL, " ");
					if (qty == NULL)
					{
						send_invalid_message_to_current_trader(t, invalid_message);
						continue;
					}
					long quantity = atol(qty);
					if (quantity < 1 || quantity > 999999)
					{
						// send invalid
						send_invalid_message_to_current_trader(t, invalid_message);
						continue;
					}
					char *p = strtok(NULL, ";");
					if (p == NULL)
					{
						send_invalid_message_to_current_trader(t, invalid_message);
						continue;
					}
					long price = atol(p);
					if (price < 1 || price > 999999)
					{
						write_to_trader(t->exchange_fd, invalid_message, strlen(invalid_message));
						send_signal_to_trader(t->trader_pid);
						free(invalid_message);
						continue;
					}
					// add order to the order book
					struct order *new_order = enqueue_order(book, order_type, order_id, product_name, quantity, price, t);
					// incerement the level of the product
					if (new_order->num_of_orders == 1 && !new_order->is_same)
					{
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
					sprintf(market_message, "MARKET %s %s %d %d;", order_type, product_name, (int)quantity, (int)price);
					for (int i = 0; i < num_of_traders; i++)
					{
						if (traders[i]->id != t->id && traders[i]->active_status)
						{
							write_to_trader(traders[i]->exchange_fd, market_message, strlen(market_message));
							send_signal_to_trader(traders[i]->trader_pid);
						}
					}
					free(market_message);
					free(invalid_message);
					// process the given order
					if (strcmp(order_type, SELL) == 0)
					{
						process_sell_order(new_order, book, t, exchanging_products, write_fill_order, send_signal_to_trader, &TOTAL_FEES);
					}
					else if (strcmp(order_type, BUY) == 0)
					{
						// printf("%s buy order processing\n", LOG_PREFIX);
						process_buy_order(new_order, book, t, exchanging_products, write_fill_order, send_signal_to_trader, &TOTAL_FEES);
					}
					// increment the trader's current order id
					t->current_order_id++;
				}
				else
				{
					// Provided command is invalid.
					write_to_trader(t->trader_fd, invalid_message, strlen(invalid_message));
					send_signal_to_trader(t->trader_pid);
					free(invalid_message);
				}
				print_orderbook(book, exchanging_products);
				print_position(exchanging_products, traders, num_of_traders);
			}
			else if (bytes_read == 0)
			{
				// end of file read reached
				// do something
			}
		}
		int active = 0;
		for (int j = 0; j < num_of_traders; j++)
		{
			if (TRADER_EXIT_STATUS == traders[j]->trader_pid)
			{
				printf("%s Trader %d disconnected\n", LOG_PREFIX, traders[j]->id);
				traders[j]->active_status = 0;
			}
			if (traders[j]->active_status)
			{
				active++;
			}
		}
		TRADER_EXIT_STATUS = -1;
		TRADER_CONNECTION = -1;
		if (active == 0)
		{
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
