		// // for (int i = 0; i < num_of_traders; i++)
		// // {
		// // 	struct trader *t1 = (struct trader *)(malloc(sizeof(struct trader)));
		// // 	t1->id = i;
		// // 	char exchange_fd[20];
		// // 	char t1_fd[20];
		// // 	sprintf(exchange_fd, FIFO_EXCHANGE, i);
		// // 	sprintf(t1_fd, FIFO_TRADER, i);
		// // 	// printf("%s\n", exchange_fd);
		// // 	// printf("%s\n", t1_fd);
		// // 	unlink(exchange_fd);
		// // 	unlink(t1_fd);
		// // 	int read_fd = mkfifo(exchange_fd, 0666);
		// // 	if (-1 == read_fd)
		// // 	{
		// // 		perror("failed to create fd");
		// // 		exit(EXIT_FAILURE);
		// // 	}
		// // 	else
		// // 	{
		// // 		printf("%s Created FIFO %s\n", LOG_PREFIX, exchange_fd);
		// // 	}
		// // 	int write_fd = mkfifo(t1_fd, 0666);
		// // 	if (-1 == write_fd)
		// // 	{
		// // 		perror("failed to create fd");
		// // 		exit(EXIT_FAILURE);
		// // 	}
		// // 	else
		// // 	{
		// // 		printf("%s Created FIFO %s\n", LOG_PREFIX, t1_fd);
		// // 	}
		// // 	int pid = fork();
		// // 	if (pid == 0)
		// // 	{
		// // 		char *trader_args[3];
		// // 		printf("%s Starting trader %d (%s)\n", LOG_PREFIX, t1->id, argv[i + 2]);
		// // 		char id[4];
		// // 		sprintf(id, "%d", t1->id);
		// // 		trader_args[0] = argv[i + 2];
		// // 		trader_args[1] = id;
		// // 		trader_args[2] = NULL;
		// // 		if (-1 == execv(argv[i + 2], trader_args))
		// // 		{
		// // 			unlink(exchange_fd);
		// // 			unlink(t1_fd);
		// // 			kill(getppid(), SIGUSR2);
		// // 			free(t1);
		// // 			exit(EXIT_FAILURE);
		// // 		}
		// // 		exit(0);
		// // 	}
        
		// // 	else if (pid > 0)
		// // 	{
		// // 		t1->trader_pid = pid;
		// // 		t1->exchange_fd = write_fd;
		// // 		t1->active_status = 1;
		// // 		t1->trader_fd = read_fd;
		// // 		if (TRADER_EXIT_STATUS == 1)
		// // 		{
		// // 			free_products(exchanging_products);
		// // 			for (int i = 0; i < num_of_traders; i++)
		// // 			{
		// // 				free(traders[i]);
		// // 			}
		// // 			free(traders);
		// // 			exit(EXIT_SUCCESS);
		// // 		}
		// // 		if (-1 == open(exchange_fd, O_WRONLY))
		// // 		{
		// // 			perror("Failed to establish connection with trader");
		// // 			exit(EXIT_FAILURE);
		// // 		}
		// // 		else
		// // 		{
		// // 			printf("%s Connected to %s\n", LOG_PREFIX, exchange_fd);
		// // 		}
		// // 		if (-1 == open(t1_fd, O_RDONLY))
		// // 		{
		// // 			perror("Failed to establish connection with trader");
		// // 			exit(EXIT_FAILURE);
		// // 		}
		// // 		else
		// // 		{
		// // 			printf("%s Connected to %s\n", LOG_PREFIX, t1_fd);
		// // 		}
		// // 		traders[i] = t1;
		// // 	}
		// // }
		// // for (int i = 0; i < num_of_traders; i++) {
		// // 	char message[128] = "MARKET OPEN";
		// // 	if (-1 == write(traders[i]->exchange_fd, message, 128)) {
		// // 		perror("Failed to write");
		// // 	}
		// // }
        // 		for (int i = 0; i < num_of_traders; i++) {
		// 	if (-1 == kill(traders[i]->trader_pid, SIGUSR1)) {
		// 		perror("Failed to send signal");
		// 	}
		// }

		// while (1)
		// {
		// 	if (TRADER_CONNECTION != -1)
		// 	{
		// 		for (int i = 0; i < num_of_traders; i++)
		// 		{
		// 			if (TRADER_CONNECTION == traders[i]->trader_pid)
		// 			{
		// 				printf("signal revieved\n");
		// 			}
		// 			// char temp[128];
		// 			// int x = read(traders[i]->trader_fd, temp, 128);
		// 			// if (x < 1) {
		// 			// 	perror("Failed to read from trader pipe");
		// 			// }
		// 			// printf("%d\n", traders[i]->trader_fd);
		// 		}
		// 	}
		// 	printf("Not happening anything\n");
		// 	break;
		// }