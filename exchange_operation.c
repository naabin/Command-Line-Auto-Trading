#include "pe_common.h"
#include "pe_exchange.h"
int compare_orders(struct order* o1, struct order *o2)
{
    return o1->price > o2->price;
}

struct order_book* create_orderbook(int order_size)
{
    struct order_book *book = (struct order_book*)calloc(1, sizeof(struct order_book));
    book->compare_orders = compare_orders;
    book->capaity = order_size;
    book->orders = (struct order**)(calloc(book->capaity, sizeof(struct order*) * book->capaity));
    return book;
}


void swap_orders(struct order_book *book, int i, int j)
{
    struct order *temp = book->orders[i];
    book->orders[i] = book->orders[j];
    book->orders[j] = temp;
}
int is_empty(struct order_book *book) {
    return book->size == 0;
}

//bootom-up heapify
void swim(int k, struct order_book *book)
{
    k = book->size - 1;
    while (k > 0 && book->compare_orders(book->orders[k], book->orders[(k - 1)/2]))
    {
        swap_orders(book, k, (k-1)/2);
        k = (k - 1)/2;
    }
}
//top dowm heapify
void sink(int k, struct order_book *book)
{
    int current = k;
    int left = 2 * k + 1;
    int right = 2 * k + 2;
    if (left < book->size && book->compare_orders(book->orders[left], book->orders[current])) {
        current = left;
    }    
    if (right < book->size && book->compare_orders(book->orders[right], book->orders[current])) {
        current = right;
    }
    if (current != k) {
        swap_orders(book, k, current);
        sink(current, book);
    }
}

struct order* dequeue(struct order_book* book)
{
    struct order *temp = book->orders[0];
    swap_orders(book, 0, book->size - 1);
    book->size--;
    sink(0, book);
    return temp;
}
void insert_same_order(struct order *order, struct order *new_order)
{
    if (order->num_of_orders == 1 || order->same_orders == NULL) {
        order->same_orders = (struct order**)calloc(1, sizeof(struct order*));
        order->same_orders[0] = new_order;
        order->num_of_orders++;
    } else {
        order->same_orders = realloc(order->same_orders, order->num_of_orders);
        order->same_orders[order->num_of_orders - 1] = new_order;
        order->num_of_orders++;
    }
}


struct order* enqueue_order(struct order_book *book, char * order_type, int order_id, char *product_name, int quantity, int price, int trader_id, struct trader *t)
{
    struct order *o = malloc(sizeof(struct order));
    o->order_type = malloc(sizeof(char) * (strlen(order_type) + 1));
    o->fulfilled = 0;
    strcpy(o->order_type, order_type);
    o->order_id = order_id;
    o->product_name = malloc(sizeof(char) * (strlen(product_name) + 1));
    strcpy(o->product_name, product_name);
    o->quantity = quantity;
    o->num_of_orders = 1;
    o->price = price;
    o->trader_id = trader_id;
    o->trader = t;
    o->same_orders = NULL;
    if (book->size == book->capaity) 
    {
        book->capaity += 1;
        book->orders = (struct order**)realloc(book->orders, sizeof(struct order*) * book->capaity);
    }
    if (book->size == 0)
    {
        book->orders[book->size] = o;
        book->size++;
        return o;
    }
    int found = 0;
    struct order *same_order = NULL;
    for (int i = 0; i < book->size; i++) {
        struct order *o1 = book->orders[i];
        if (o->price == o1->price && (strcmp(o->product_name, o1->product_name) == 0) && (o->quantity == o1->quantity)
            && (strcmp(o->order_type, o1->order_type) == 0)) {
            same_order = o1;
            found = 1;
        }    
    }
    if (found)
    {
        insert_same_order(same_order, o);
        return same_order;
    } 
    else {
        book->orders[book->size] = o;
        book->size++; 
        swim(book->size, book);   
    }
    return o;
}

int search_product_in_book(char *product_name, struct order_book* book) {
    for (int i = 0; i < book->size; i++) {
        char *p_name = book->orders[i]->product_name;
        if (strcmp(product_name, p_name) == 0) {
            return 1;
        }
    }
    return 0;
}



void print_orderbook(struct order_book *book, struct products* available_products) 
{
    printf("%s\t--ORDERBOOK--\n", LOG_PREFIX);
    for (int i = 0; i < available_products->num_of_products; i++) {
        struct product *p = available_products->itms[i];
        char *p_name = p->product_name;
        printf("%s\tProduct: %s; Buy levels: %d; Sell levels: %d\n", LOG_PREFIX, p_name, p->buy_level, p->sell_level);
        int original_size = book->size;
        while (!is_empty(book))
        {
            struct order *o = dequeue(book);
            if (!o->fulfilled) {
            if (strcmp(o->product_name, p_name) == 0) {
                if (o->num_of_orders > 1) {
                    printf("%s\t\t%s %d @ $%d (%d orders)\n",LOG_PREFIX, o->order_type, o->quantity * o->num_of_orders, o->price, o->num_of_orders);    
                } else {
                    printf("%s\t\t%s %d @ $%d (%d order)\n", LOG_PREFIX, o->order_type, o->quantity, o->price, o->num_of_orders);
                }
            }
            }
        }
        book->size = original_size;
        swim(book->size, book);
    }
}

void print_position(struct products * ex_products, struct trader **traders, int num_of_traders) {
    printf("%s\t--POSITIONS--\n", LOG_PREFIX);
    for (int j = 0; j < num_of_traders; j++) {
        printf("%s\tTrader %d: ",LOG_PREFIX, traders[j]->trader_fifo_id);
        for (int k = 0; k < ex_products->num_of_products; k++) {
            if (k == ex_products->num_of_products - 1) {
                printf("%s %d ($%d)\n", ex_products->itms[k]->product_name, traders[j]->position_qty[k], traders[j]->position_price[k]);    
            } else {
                printf("%s %d ($%d), ", ex_products->itms[k]->product_name, traders[j]->position_qty[k], traders[j]->position_price[k]);
            }
        }
    }
}
void decrement_level(struct products *a_products, struct order *c_order) {
    for (int i = 0; i < a_products->num_of_products; i++) {
        char *p_name = a_products->itms[i]->product_name;
        if (strcmp(p_name, c_order->product_name) == 0) {
            if (strcmp(SELL, c_order->order_type) == 0) {
                a_products->itms[i]->sell_level--;
            }
            else {
                a_products->itms[i]->buy_level--;
            }
        }
    }
}

int cancel_order(struct order_book *book, int order_id, struct trader* t, struct products *available_products, struct trader **traders, int num_of_traders) {
    if ((order_id < 0) && (order_id > book->size)) {
        return 0;
    }
    int found = 0;
    struct order *deleting_order = NULL;
    for (int i = 0; i < book->size; i++) {
        if (book->orders[i]->num_of_orders > 1) {
            struct order temp = *book->orders[i];
            int num_of_orders = book->orders[i]->num_of_orders;
            int index = 0;
            while (num_of_orders >= 1) {
                if ((temp.order_id == order_id) && (temp.trader_id == t->id) && (!temp.fulfilled)) {
                    deleting_order = &temp;
                    book->orders[i]->same_orders[index]->fulfilled = 1;
                    break;
                }
                if(temp.num_of_orders == 1) {
                    break;
                }
                temp.num_of_orders = num_of_orders - 1;
                temp = *book->orders[i]->same_orders[index++];
                num_of_orders--;
            }
            found = deleting_order != NULL ? 1 : 0;
            break;
        }
        else if ((book->orders[i]->order_id == order_id) && (!book->orders[i]->fulfilled)) {
            // check if the trader owner matches with order
            if (book->orders[i]->trader_id != t->id) {
                return 0;
            }
            book->orders[i]->fulfilled = 1;
            deleting_order = book->orders[i];
            found = 1;
            break;
        }
    }
    if (found) {
        char *message = malloc(sizeof(char) * 1024);
        sprintf(message, "MARKET %s %s %d %d;", deleting_order->order_type, deleting_order->product_name, deleting_order->order_id, 0);
        for (int i = 0; i < num_of_traders; i++) {
            if (traders[i]->id != deleting_order->trader_id && traders[i]->active_status) {
                if (-1 == write(traders[i]->exchange_fd, message, strlen(message))) {
                    perror("write error broadcasting cancel: ");
                }
                if (-1 == kill(traders[i]->trader_pid, SIGUSR1)) {
                    perror("kill error while broadcasting cancelled order: ");
                }
            }
        }
        free(message);
        decrement_level(available_products, deleting_order);
        return 1;
    } else {
        return 0;
    }
}

int update_order(struct order_book* book, int order_id, int new_quanity, int new_price, struct trader *t) {
    printf("does it come here: 252\n");
    if ((order_id < 0)) return 0;
    printf("does it come here 254\n");
    if (((new_quanity < 1) && (new_quanity > 999999)) && ((new_price < 1) && (new_price > 999999))) return 0;
    printf("does it come here 256\n");
    for (int i = 0; i < book->size; i++) {
        if (book->orders[i]->num_of_orders > 1) {
            struct order temp = *book->orders[i];
            int index = 0;
            int num_of_orders = book->orders[i]->num_of_orders;
            while (num_of_orders >= 1) {
                if ((temp.order_id == order_id) && (temp.trader_id == t->id) && (!temp.fulfilled)) {
                    if ((temp.quantity == new_quanity) && (temp.price == new_price)) return 0;
                    book->orders[i]->same_orders[index]->fulfilled = 1;
                    enqueue_order(book, temp.order_type, temp.order_id, temp.product_name, new_quanity, new_price, temp.trader_id, temp.trader);
                    return 0;
                }
                temp = *book->orders[i]->same_orders[index++];
                num_of_orders--;
            }
        }
        else if (book->orders[i]->order_id == order_id) {
            printf("does it come here 273\n");
            if (book->orders[i]->trader_id != t->id) {
                printf("armageddon\n");
                return 0;
            }
            if (book->orders[i]->fulfilled){
                printf("armageddon 2\n");
                return 0;
            } 
            book->orders[i]->quantity = new_quanity;
            book->orders[i]->price = new_price;
            return 1;
        }
    }
    return 0;
}

int check_if_product_exist(struct products *available_products, char* new_product_name) {
    if (strlen(new_product_name) <= 0) return 0;
    for (int i = 0; i < available_products->num_of_products; i++) {
        if (strcmp(available_products->itms[i]->product_name, new_product_name) == 0) {
            return 1;
        }
    }
    return 0;
}

void increment_level(struct products* available_products, char* order_type, char* product_name)
{
    for (int i = 0; i < available_products->num_of_products; i++) {
        char *p_name = available_products->itms[i]->product_name;
        if (strcmp(p_name, product_name) == 0) {
            if (strcmp("BUY", order_type) == 0) {
                available_products->itms[i]->buy_level++;
            } else {
                available_products->itms[i]->sell_level++;
            }
        }
    }
}

int get_traders_position_index(struct products *available_products, struct order *o) {
    for (int i = 0; i < available_products->num_of_products; i++) {
        if (strcmp(available_products->itms[i]->product_name, o->product_name) == 0) {
            return i;
        }
    }
    return -1;
}
void log_match_order_to_stdout(char *order_type, struct order *o, struct order *new_order, int qty, int value, int fee, struct products *available_products) {
    int trader_pos = get_traders_position_index(available_products, o);
    if (strcmp(SELL, order_type) == 0) {
        o->trader->position_qty[trader_pos] += qty;
        o->trader->position_price[trader_pos] -= value;
    }
    if (strcmp(BUY, order_type) == 0) {
        o->trader->position_qty[trader_pos] -= qty;
        o->trader->position_price[trader_pos] += value;
    }
    
    printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%d, fee: $%d.\n",
        LOG_PREFIX, o->order_id, o->trader_id, new_order->order_id, new_order->trader_id, value, fee);
    if (strcmp(order_type, SELL) == 0) {
        new_order->trader->position_qty[trader_pos] -= qty;
        new_order->trader->position_price[trader_pos] += (value - fee);
    } else if (strcmp(order_type, BUY) == 0) {
        new_order->trader->position_qty[trader_pos] += qty;
        new_order->trader->position_price[trader_pos] -= (value + fee);
    }
    
    
}
void process_order_for_sell(struct order* current_order, struct order *new_order, struct products *available_products, int *fees, write_fill fill_message, send_sig signal_traders) {
    int qty = new_order->quantity < current_order->quantity ? new_order->quantity: current_order->quantity;
    int value = qty * current_order->price;
    int fee = roundl(value * (FEE_PERCENTAGE * 0.01));
    *fees += fee;
    log_match_order_to_stdout(SELL, current_order, new_order, qty, value, fee, available_products);
    if (current_order->trader->active_status) {
            fill_message(current_order->trader->exchange_fd, current_order->order_id, qty);
            signal_traders(current_order->trader->trader_pid);
    }
    if (new_order->trader->active_status) {
        fill_message(new_order->trader->exchange_fd, new_order->order_id, qty);
        signal_traders(current_order->trader->trader_pid);
    }
}

void process_sell_order(struct order *new_order, struct order_book *book, struct trader *t, 
    struct products *available_products, write_fill fill_message, send_sig signal_traders, int *fees)
{
    int o_size = book->size;
    while (book->size - 1 >= 0) {
        if (new_order->fulfilled) {
            decrement_level(available_products, new_order);
            break;
        }
        struct order * max_buy_order = dequeue(book);
        if ((strcmp(max_buy_order->order_type, "SELL") == 0) || (max_buy_order->trader_id == t->id) || max_buy_order->fulfilled) {
            continue;
        }
        if (new_order->price > max_buy_order->price) break;
        //This will exit the loop after filling orders
        if (max_buy_order->quantity > new_order->quantity) {
            process_order_for_sell(max_buy_order, new_order, available_products, fees, fill_message, signal_traders);
            max_buy_order->quantity = max_buy_order->quantity - new_order->quantity;
            new_order->fulfilled = 1;
            decrement_level(available_products, new_order);
            break;
        }
        else if (max_buy_order->quantity < new_order->quantity) {
                if (max_buy_order->num_of_orders > 1) {
                    // use stack space to process multiple orders
                    struct order same_order = *max_buy_order;
                    int index = 0;
                    int num_of_orders = max_buy_order->num_of_orders;
                    while(num_of_orders >= 1) {
                        if (new_order->quantity <= 0) {
                            new_order->fulfilled = 1;
                            break;
                        }
                        if (same_order.fulfilled) {
                            if (num_of_orders > 1) {
                                same_order = *max_buy_order->same_orders[index];
                                index++;
                                num_of_orders--;
                            } else break;
                        }
                        process_order_for_sell(&same_order, new_order, available_products, fees, fill_message, signal_traders);
                        //lookout for partially filled order
                        if (new_order->quantity < same_order.quantity && new_order->quantity > 0) {
                            // printf("second if\n");
                            if (!same_order.fulfilled && same_order.quantity > 0) {
                                same_order.quantity -= new_order->quantity;
                                process_order_for_sell(&same_order, new_order, available_products, fees, fill_message, signal_traders);
                                //TODO: enqueue partially filled order
                                new_order->fulfilled = 1;
                                break;
                            }
                        }
                        if (same_order.num_of_orders == 1) {
                            max_buy_order->fulfilled = 1;
                            new_order->quantity -= same_order.quantity;
                            decrement_level(available_products, max_buy_order);
                            break;
                        }
                        new_order->quantity -= same_order.quantity;
                        same_order = *max_buy_order->same_orders[index++];
                        num_of_orders--;
                        same_order.num_of_orders = num_of_orders;
                    }
                } else {
                    process_order_for_sell(max_buy_order, new_order, available_products, fees, fill_message, signal_traders);
                    new_order->quantity -= max_buy_order->quantity;
                    max_buy_order->fulfilled = 1;
                    decrement_level(available_products, max_buy_order);
                }
        } else {
            process_order_for_sell(max_buy_order, new_order, available_products, fees, fill_message, signal_traders);
            max_buy_order->fulfilled = 1;
            new_order->fulfilled = 1;
            decrement_level(available_products, max_buy_order);
        }
    }
    book->size = o_size;
    swim(book->size, book);
}

void process_order_for_buy(struct order* current_order, struct order* new_order, struct products* available_products, int *fees, write_fill fill_message, send_sig signal_traders)
{
    int qty = new_order->quantity < current_order->quantity ? new_order->quantity: current_order->quantity;
    int value = qty * current_order->price;
    int fee = roundl(value * (FEE_PERCENTAGE * 0.01));
    *fees += fee;
    log_match_order_to_stdout(BUY, current_order, new_order, qty, value, fee, available_products);
    if (current_order->trader->active_status) {
        fill_message(current_order->trader->exchange_fd, current_order->order_id, qty);
        signal_traders(current_order->trader->trader_pid);
    }
    if (new_order->trader->active_status) {
        fill_message(new_order->trader->exchange_fd, new_order->order_id, qty);
        signal_traders(current_order->trader->trader_pid);
    }
}

void process_buy_order(struct order *new_order, struct order_book *book, struct trader *t, struct products *available_products, 
    write_fill fill_message, send_sig signal_traders, int *fees) {
        //TODO: not sure if this will find the smallest sell order
        int o_size = book->size;
        // while(o_size >= 0) {
        //     struct order low_sell_
        // }
        for (int i = o_size-1; i >= 0; i--) {
            struct order *current_order = book->orders[i];
            if (strcmp(current_order->order_type, BUY) == 0 || current_order->trader_id == t->id || (strcmp(current_order->product_name, new_order->product_name) != 0)) {
                continue;
            }
            else if (current_order->price <= new_order->price) {
                if (current_order->quantity > new_order->quantity) {
                    process_order_for_buy(current_order, new_order, available_products, fees, fill_message, signal_traders);
                    current_order->quantity = current_order->quantity - new_order->quantity;
                    new_order->fulfilled = 1;
                    decrement_level(available_products, new_order);
                    break;
                } else if (current_order->quantity < new_order->quantity) {
                    if (current_order->num_of_orders > 1) {
                        struct order same_order = *current_order;
                        int index = 0;
                        int num_of_orders = same_order.num_of_orders;
                        while (num_of_orders >= 1) {
                            if (new_order->quantity <= 0) {
                                new_order->fulfilled = 1;
                                decrement_level(available_products, new_order);
                                break;
                            }
                            if (same_order.fulfilled) {
                                if (num_of_orders > 1) {
                                    same_order = *current_order->same_orders[index];
                                    index++;
                                    num_of_orders--;
                                } else break;
                            }
                            process_order_for_buy(&same_order, new_order, available_products, fees, fill_message, signal_traders);
                            if (new_order->quantity < same_order.quantity && new_order->quantity > 0) {
                                // printf("second if\n");
                                if (!same_order.fulfilled && same_order.quantity > 0) {
                                    same_order.quantity -= new_order->quantity;
                                    process_order_for_buy(&same_order, new_order, available_products, fees, fill_message, signal_traders);
                                    //TODO: enqueue partially filled order
                                    new_order->fulfilled = 1;
                                    break;
                                }
                            }
                            new_order->quantity -= same_order.quantity;
                            same_order = *current_order->same_orders[index++];
                            num_of_orders--;
                            same_order.num_of_orders = num_of_orders;
                        }
                    }   
                    else {
                        process_order_for_buy(current_order, new_order, available_products, fees, fill_message, signal_traders);
                        current_order->fulfilled = 1;
                        new_order->quantity -= current_order->quantity;
                        decrement_level(available_products, current_order);
                    }
                } else if (current_order->quantity == new_order->quantity) {
                    process_order_for_buy(current_order, new_order, available_products, fees, fill_message, signal_traders);
                    new_order->fulfilled = 1;
                    current_order->fulfilled = 1;
                    decrement_level(available_products, new_order);
                    decrement_level(available_products, current_order);
                }
            }
            
        }
}

void free_orderbook(struct order_book* book)
{
    while (!is_empty(book))
    {
        struct order *o = dequeue(book);
        if ((o->num_of_orders > 1) && (o->same_orders != NULL)) {
            for (int i = 0; i < o->num_of_orders - 1; i++) {
                struct order *s_order = o->same_orders[i];
                free(s_order->product_name);
                free(s_order->order_type);
                free(s_order);
            }
            free(o->same_orders);
            free(o->product_name);
            free(o->order_type);
            free(o);
        } else {
            free(o->order_type);
            free(o->product_name);
            free(o);
        }
    }
    free(book->orders);
    free(book);
}