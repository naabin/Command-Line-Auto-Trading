1. Describe how your exchange works.
    Exchange program acts as a server and as a platform for traders where traders are binary executables passed from the 
    command line argument and then it creates the channel to communicate between multiple traders via
    named pipes with the help of signals. The process of how it works is as follows in a sequential order
    1. The exchange program expects products file of (.txt) format and desired number of trader binaries
    2. Reads the file and sets the products what traders can trade it with
    3. Registers the signals where traders can only communicate with signals such as SIGUSR1
    4. Creates child processes by forking the main process for each trader
    5. It then creates two named pipes and traders are given specific ids: one for reading and one for writing 
        to each trader
    6. Once traders are connected to the Exchange, it will setup the reading and writing mechanism with 
        traders to listen to the event sent by traders via signals with the help of epoll where traders can communicate via
        exchange to each other.
    7. There are four main functionalities of where exchange will handle and the are and traders has to adhere to
        the following format.
        BUY order_id product_name quatity price;
        SELL order_id product_name quatity price;
        AMEND order_id qty price;
        CANCEL order_id;
    8. Exchange will parse all those given commands and try to convey the message to other traders. At the same time
        Exchange will also try to attempt to match the orders between BUY and SELL and viceversa. If there are any matched
        orders then it will process those orders and write to the traders that their orders has been fulfilled. As a service
        it will charge 1% of the processed order.
    9. Orders are also reported to stdout in a descending order. To effectively process the order and match the order, priority 
        queue data structure is implemented internally to store the orders. 
    10. Once trader indicates that it will disconnect then it will gracefully disconnect the trader and continue communicating 
        with other traders. If there are no active traders then program will be terminated and report how much fee it has managed
        to collect. 
2. Describe your design decisions for the trader and how it's fault-tolerant.
    Auto trader will setup the signal using sigaction struct and it will keep listening to sell orders written by the
    exchange. Just after entering the event loop it will pause for the signal to arrive before it can read from the exhange file descriptor
    .If there as a SELL order then it will write the BUY order to the exchange and send the signal until signal gets send successfully to the 
    exchange.
3. Describe your tests and how to run them.
    TODO:
