#include "pe_common.h"
#include "pe_exchange.h"
int compare_orders(struct order* o1, struct order *o2)
{
    return o1->price < o2->price;
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
    while (k > 1 && book->compare_orders(book->orders[k/2], book->orders[k]))
    {
        swap_orders(book, k/2, k);
        k = k/2;
    }
}
//top dowm heapify
void sink(int k, struct order_book *book)
{
    while (2*k <= book->size)
    {
        int j = 2*k;
        if (j < book->size && book->compare_orders(book->orders[j], book->orders[j+1])) j++;
        if (!book->compare_orders(book->orders[k], book->orders[j])) break;
        swap_orders(book, k, j);
        k = j;
    }
}

struct order* dequeue(struct order_book* book)
{
    struct order *temp = book->orders[1];
    swap_orders(book, 1, book->size--);
    sink(1, book);
    return temp;
}

struct order* enqueue_order(struct order_book *book, char * order_type, int order_id, char *product_name, int quantity, int price, int trader_id, struct trader *t)
{
    struct order *o = malloc(sizeof(struct order));
    o->order_type = malloc(sizeof(char) * (strlen(order_type) + 1));
    o->ids_length = 0;
    strcpy(o->order_type, order_type);
    o->order_id = order_id;
    o->product_name = malloc(sizeof(char) * (strlen(product_name) + 1));
    strcpy(o->product_name, product_name);
    o->quantity = quantity;
    o->num_of_orders = 1;
    o->price = price;
    o->trader_id = trader_id;
    o->trader = t;
    if (book->size == book->capaity) 
    {
        book->capaity *= 2;
        book->orders = (struct order**)realloc(book->orders, sizeof(struct order*) * book->capaity);
    }
    if (book->size == 0)
    {
        book->orders[++book->size] = o;
        return o;
    }
    else
    {
        int found = 0;
        int found_index;
        for (int i = 1; i <= book->size; i++) {
            struct order *o1 = book->orders[i];
            if (o->price == o1->price && strcmp(o->product_name, o1->product_name) == 0 && (o->quantity == o1->quantity)
             && (strcmp(o->order_type, o1->order_type) == 0)) {
                found = 1;
                found_index = i;
            }    
        }
        if (found)
        {
            int id_length = book->orders[found_index]->ids_length;
            if (id_length == 0) {
                id_length = 2;
                book->orders[found_index]->ids = malloc(sizeof(int) * id_length);
                book->orders[found_index]->ids[0] = book->orders[found_index]->order_id;
                book->orders[found_index]->ids[id_length - 1] = o->order_id;
                book->orders[found_index]->ids_length = id_length;
            } else if (id_length >= 2) {
                book->orders[found_index]->ids = realloc(book->orders[found_index]->ids, sizeof(int) * (id_length + 1));
                book->orders[found_index]->ids[id_length] = o->order_id;
                book->orders[found_index]->ids_length++;
            }
            // book->orders[found_index].quantity += o.quantity;
            free(o->order_type);
            free(o->product_name);
            free(o);
            book->orders[found_index]->num_of_orders += 1;
            return book->orders[found_index];
        } 
        else {
            book->orders[++book->size] = o;
            // memcpy(book->orders[++book->size], &o, sizeof(struct order));
            swim(book->size, book);    
        }
        
    }
    return o;
}

int search_product_in_book(char *product_name, struct order_book* book) {
    for (int i = 1; i <= book->size; i++) {
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
            if (strcmp(o->product_name, p_name) == 0) {
                if (o->num_of_orders > 1) {
                    printf("%s\t\t%s %d @ $%d (%d orders)\n",LOG_PREFIX, o->order_type, o->quantity * o->num_of_orders, o->price, o->num_of_orders);    
                } else {
                    printf("%s\t\t%s %d @ $%d (%d order)\n", LOG_PREFIX, o->order_type, o->quantity, o->price, o->num_of_orders);
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

int cancel_order(struct order_book *book, int order_id, struct trader* t, struct products *available_products) {
    if (order_id < 0 || order_id > book->size) {
        return 0;
    }
    // locate the index of the order
    int index = -1;
    char *p_name = malloc(sizeof(char) * 1024);
    char *order_type = malloc(sizeof(char) * 1024);
    for (int i = 1; i <= book->size; i++) {
        if (book->orders[i]->num_of_orders > 1) {
            for (int j = 0; j < book->orders[i]->ids_length; j++) {
                int id = book->orders[i]->ids[j];
                if (id == order_id) {
                    // check if the trader owner matches with order
                    if (book->orders[i]->trader_id != t->id) {
                        return 0;
                    }
                    strcpy(p_name, book->orders[i]->product_name);
                    strcpy(order_type, book->orders[i]->order_type);
                    index = i;
                }
            }
        }
        else if (book->orders[i]->order_id == order_id) {
            // check if the trader owner matches with order
            if (book->orders[i]->trader_id != t->id) {
                return 0;
            }
            index = i;
            strcpy(p_name, book->orders[i]->product_name);
            strcpy(order_type, book->orders[i]->order_type);
            break;
        }
    }
    if (index > 0) {
        // dealing with multiple same orders
        if (book->orders[index]->num_of_orders > 1) {
            int order_id_index;
            book->orders[index]->quantity /= book->orders[index]->ids_length;
            struct order *o = book->orders[index];
            for (int i = 0; i < book->orders[index]->ids_length; i++) {
                int id = o->ids[i];
                if (id == order_id) {
                    order_id_index = i;
                    break;
                }
            }
            for (int i = order_id_index; i < book->orders[index]->ids_length - 1; i++) {
                book->orders[index]->ids[i] = book->orders[index]->ids[i + 1];
                if (book->orders[index]->order_id == order_id) {
                    book->orders[index]->order_id = book->orders[index]->ids[i + 1];
                }
            }
            book->orders[index]->num_of_orders -= 1;
            book->orders[index]->ids_length--;
            if (book->orders[index]->num_of_orders == 1) {
                free(book->orders[index]->ids);
            }
        } else {
            free(book->orders[index]->product_name);
            free(book->orders[index]->order_type);
            free(book->orders[index]);
            for (int i = index; i <= book->size-1; i++) {
                book->orders[i] = book->orders[i + 1];
            }
            book->size--;
        }
        for (int i = 0; i < available_products->num_of_products; i++) {
            char *p = available_products->itms[i]->product_name;
            if (strcmp(p, p_name) == 0) {
                // printf("%s\n", order_type);
                if (strcmp(order_type, "BUY") == 0) {
                    available_products->itms[i]->buy_level--;
                }
                else if (strcmp(order_type, "SELL") == 0) {
                    available_products->itms[i]->sell_level--;
                }
            }
        }
        free(order_type);
        free(p_name);
        return 1;
    } else {
        free(order_type);
        free(p_name);
        return 0;
    }
}

int update_order(struct order_book* book, int order_id, int new_quanity, int new_price, struct trader *t) {
    if (order_id < 0 || order_id > book->size) return 0;
    if ((new_quanity < 1 || new_quanity > 999999) || (new_price < 1) || (new_price > 999999)) return 0;
    for (int i = 1; i <= book->size; i++) {
        if (book->orders[i]->num_of_orders > 1) {
            int id;
            int index = -1;
            for (int j = 0; j < book->orders[i]->ids_length; i++) {
                id = book->orders[i]->ids[j];
                if (id == order_id) {
                    if (book->orders[i]->trader_id != t->id) {
                        return 0;
                    }
                    index = i;
                }
            }
            if (index > 0) {
                for (int k = index; k < book->orders[index]->ids_length - 1; k++) {
                    book->orders[index]->ids[k] = book->orders[index]->ids[k + 1];
                }
                book->orders[index]->ids_length--;
                book->orders[index]->num_of_orders--;
                if (book->orders[index]->num_of_orders == 1) {
                    free(book->orders[index]->ids);
                }
                struct order *o = book->orders[index];
                enqueue_order(book, o->order_type, id, o->product_name, new_quanity, new_price, o->trader_id, t);
                return 1;
            }
        }
        else if (book->orders[i]->order_id == order_id) {
            if (book->orders[i]->trader_id != t->id) {
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

void private_enqueue(struct order_book *book, struct order *o) {
    if (book->size == book->capaity) 
    {
        book->capaity *= 2;
        book->orders = (struct order**)realloc(book->orders, sizeof(struct order*) * book->capaity);
    }
    if (book->size == 0)
    {
        book->orders[++book->size] = o;
    } else {
        book->orders[++book->size] = o;
            // memcpy(book->orders[++book->size], &o, sizeof(struct order));
        swim(book->size, book); 
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
void log_match_order_to_stdout(struct order *o, struct order *new_order, int qty, int value, int fee, struct products *available_products) {
    int trader_pos = get_traders_position_index(available_products, o);
    o->trader->position_qty[trader_pos] += qty;
    o->trader->position_price[trader_pos] -= value;
    printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%d, fee: $%d.\n",
        LOG_PREFIX, o->order_id, o->trader_id, new_order->order_id, new_order->trader_id, value, fee);
    new_order->trader->position_qty[trader_pos] -= qty;
    new_order->trader->position_price[trader_pos] += (value - fee);
    
}

void process_sell_order(struct order *new_order, struct order_book *book, struct trader *t, 
    struct products *available_products, write_fill fill_message, send_sig signal_traders, int *fees)
{
    struct order_book *dup_book = create_orderbook(10);
    while (!is_empty(book)) {
        struct order *o = dequeue(book);
        if ((strcmp(o->order_type, "SELL") == 0) || (o->trader_id == t->id)) {
            private_enqueue(dup_book, o);
        } else if (o->price > new_order->price) {
            // while (1) {
                if (o->quantity > new_order->quantity) {
                    // int qty = o->quantity - new_order->quantity;
                    int value = new_order->quantity * o->price;
                    int fee = round(new_order->quantity * (FEE_PERCENTAGE * 0.01));
                    *fees += fee;
                    //update the existing order
                    log_match_order_to_stdout(o, new_order, new_order->quantity, value, fee, available_products);
                    //send fill order
                    fill_message(o->trader->exchange_fd, o->order_id, new_order->quantity);
                    signal_traders(o->trader->trader_pid);
                    fill_message(new_order->trader->exchange_fd, new_order->trader_id, new_order->quantity);
                    signal_traders(o->trader->trader_pid);
                    update_order(book, o->order_id, o->quantity - new_order->quantity, o->price, o->trader);
                    // o->quantity = o->quantity - new_order->quantity;
                    // remove new order
                    cancel_order(book, new_order->order_id, new_order->trader, available_products);
                    private_enqueue(dup_book, o);
                    break;
                }
                else if (o->quantity < new_order->quantity) {
                    while (o->num_of_orders >= 1) {
                        int r_qty = new_order->quantity - o->quantity;
                        int value = o->quantity * o->price;
                        int fee = roundl(value * (FEE_PERCENTAGE/100.0));
                        *fees += fee;
                        log_match_order_to_stdout(o, new_order, o->quantity, value, fee, available_products);
                        //send the fill order
                        fill_message(o->trader->exchange_fd, o->order_id, o->quantity);
                        signal_traders(o->trader->trader_pid);
                        fill_message(new_order->trader->exchange_fd, new_order->trader_id, o->quantity);
                        signal_traders(o->trader->trader_pid);
                        //update the new order quantity
                        // update_order(book, new_order->order_id, r_qty, new_order->price, new_order->trader);
                        new_order->quantity = r_qty;
                        // remove the fulfilled order
                        if (o->num_of_orders > 1) {
                            // int *ids = o->ids;
                            for (int i = 1; i < o->ids_length; i++) {
                                o->ids[i - 1] = o->ids[i];
                            }
                            o->num_of_orders--;
                            o->ids_length--;
                            o->order_id = o->ids[0];
                            // update_order(book, o->ids[0], o->quantity, o->price, o->trader);
                            
                        } else {
                            cancel_order(book, o->order_id, o->trader, available_products);
                            break;
                        }
                    }
                } else if (o->quantity == new_order->quantity) {
                    int value = o->quantity * o->price;
                    int fee = roundl(value * (FEE_PERCENTAGE/100.0));
                    *fees += fee;
                    //stdout the match order
                    log_match_order_to_stdout(o, new_order, o->quantity, value, fee, available_products);
                    //send fill order
                    fill_message(o->trader->exchange_fd, o->order_id, o->quantity);
                    signal_traders(o->trader->trader_pid);
                    fill_message(new_order->trader->exchange_fd, new_order->trader_id, o->quantity);
                    signal_traders(o->trader->trader_pid);
                    // remove both order from the orderbook
                    cancel_order(book, new_order->order_id, new_order->trader, available_products);
                    cancel_order(book, o->order_id, o->trader, available_products);
                    break;
                }
            // }
        } 
        else {
            private_enqueue(dup_book, o);
        }
    }
    while (!is_empty(dup_book)) {
        struct order *o = dequeue(dup_book);
        private_enqueue(book, o);
    }
    free_orderbook(dup_book);  
}

void delete_order(struct order_book *book, struct order *o) {
    int index = -1;
    for (int i = 1; i <= book->size; i++) {
        if (book->orders[i]->order_id == o->order_id) {
            if (book->orders[i]->num_of_orders > 1) {
                // int *ids = book->orders[i]->ids;
                int len = book->orders[i]->ids_length;
                for (int j = 0; j < len - 1; j++) {
                    book->orders[j] = book->orders[j + 1];
                }
                book->orders[i]->ids_length--;
                book->orders[i]->num_of_orders--;
                if (book->orders[i]->num_of_orders == 1) {
                    free(book->orders[i]->ids);
                }
            } else {
                index = i;
            }
        }
    }
    if (index > 0) {
        for (int i = index; i < book->size; i++) {
            book->orders[i] = book->orders[i + 1];
        }
    }
}

void free_orderbook(struct order_book* book)
{
    while (!is_empty(book))
    {
        struct order *o = dequeue(book);
        if (o->num_of_orders > 1) {
            free(o->ids);
        }
        free(o->product_name);
        free(o->order_type);
        free(o);
    }
    free(book->orders);
    free(book);
}