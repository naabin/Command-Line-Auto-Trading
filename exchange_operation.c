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

int insert_same_order(struct order **same_order, struct order *new_order) 
{
    if (*(same_order) == NULL) {
        *same_order = new_order;
        return 1;
    }
    struct order *temp = *same_order;
    int curr = 1;
    while (temp->next != NULL) {
        temp = temp->next;
        if (temp->next == NULL) break;
        curr += 1;
    }
    temp->next = new_order;
    return curr;
}

struct order* detach_order_from_same_order(struct order **same_order, int deleting_order_id, int return_head)
{
    struct order *prev = NULL;
    struct order *temp = *same_order;
    int num_of_orders = (*same_order)->num_of_orders;
    if (temp != NULL && temp->order_id == deleting_order_id) {
        struct order *t = *same_order;
        *same_order = (*same_order)->next;
        temp->next->num_of_orders = num_of_orders - 1;
        if (return_head){
            free(temp->order_type);
            free(temp->product_name);
            free(temp);
            return *same_order;
        } 
        return t;
    }
    while (temp != NULL && temp->order_id != deleting_order_id) {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL) return NULL;
    (*same_order)->num_of_orders = num_of_orders - 1;
    prev->next = temp->next;
    return temp;
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
    o->next = NULL;
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
        // int found_index = -1;
        struct order *same_order = NULL;
        for (int i = 1; i <= book->size; i++) {
            struct order *o1 = book->orders[i];
            if (o->price == o1->price && strcmp(o->product_name, o1->product_name) == 0 && (o->quantity == o1->quantity)
             && (strcmp(o->order_type, o1->order_type) == 0)) {
                same_order = o1;
                // found_index = i;
                found = 1;
            }    
        }
        if (found)
        {
            int num_of_orders = insert_same_order(&same_order, o);
            same_order->num_of_orders = num_of_orders + 1;
            return same_order;
        } 
        else {
            book->orders[++book->size] = o;
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

int cancel_order(struct order_book *book, int order_id, struct trader* t, struct products *available_products, struct trader **traders, int num_of_traders) {
    if (order_id < 0 && order_id > book->size) {
        return 0;
    }
    int found = 0;
    int delete_index = 0;
    struct order *deleting_order = NULL;
    for (int i = 1; i <= book->size; i++) {
        if (book->orders[i]->num_of_orders > 1) {
            for(struct order *o=book->orders[i]; o != NULL; o = o->next) {
                if (order_id == o->order_id) {
                    if (o->trader->id != t->id) {
                        return 0;  
                    }
                    deleting_order = detach_order_from_same_order(&book->orders[i], o->order_id, 0);
                    found = 1;
                    break;
                }
            }
        }
        else if (book->orders[i]->order_id == order_id) {
            // check if the trader owner matches with order
            if (book->orders[i]->trader_id != t->id) {
                return 0;
            }
            deleting_order = book->orders[i];
            found = 1;
            delete_index = i;
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
        for (int i = 0; i < available_products->num_of_products; i++) {
            char *p = available_products->itms[i]->product_name;
            if (strcmp(p, deleting_order->product_name) == 0) {
                if (strcmp(deleting_order->order_type, "BUY") == 0) {
                    available_products->itms[i]->buy_level--;
                }
                else if (strcmp(deleting_order->order_type, "SELL") == 0) {
                    available_products->itms[i]->sell_level--;
                }
            }
        }
        if (delete_index) {
            // printf("%d\n", delete_index);
            for (int i = delete_index; i < book->size; i++) {
                book->orders[i] = book->orders[i + 1];
            }
            book->size--;
        }
        free(deleting_order->product_name);
        free(deleting_order->order_type);
        free(deleting_order);
        return 1;
    } else {
        return 0;
    }
}

int update_order(struct order_book* book, int order_id, int new_quanity, int new_price, struct trader *t) {
    if ((order_id < 0) && (order_id > book->size)) return 0;
    if (((new_quanity < 1) && (new_quanity > 999999)) && ((new_price < 1) && (new_price > 999999))) return 0;
    for (int i = 1; i <= book->size; i++) {
        if (book->orders[i]->num_of_orders > 1) {
            for (struct order *o = book->orders[i]; o != NULL; o = o->next) {
                if (o->order_id == order_id) {
                    if (o->trader->id != t->id) {
                        return 0;
                    }
                    //same order
                    if ((o->quantity == new_quanity) && (o->price == new_price)) return 1;
                    struct order *diff_order = detach_order_from_same_order(&book->orders[i], o->order_id, 0);
                    enqueue_order(book, diff_order->order_type, order_id, diff_order->product_name, new_quanity, new_price, diff_order->trader_id, diff_order->trader);
                    free(diff_order->product_name);
                    free(diff_order->order_type);
                    free(diff_order);
                    return 1;
                }
            }
        }
        else if (book->orders[i]->order_id == order_id) {
            if (book->orders[i]->trader->id != t->id) {
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
            fill_message(current_order->trader->exchange_fd, current_order->order_id, current_order->quantity);
            signal_traders(current_order->trader->trader_pid);
    }
    if (new_order->trader->active_status) {
        fill_message(new_order->trader->exchange_fd, new_order->order_id, current_order->quantity);
        signal_traders(current_order->trader->trader_pid);
    }
}

void process_sell_order(struct order *new_order, struct order_book *book, struct trader *t, 
    struct products *available_products, write_fill fill_message, send_sig signal_traders, int *fees)
{
    struct order_book *dup_book = create_orderbook(10);
    while(!is_empty(book)) {
        struct order *current_order = dequeue(book);
        if ((strcmp(current_order->order_type, "SELL") == 0) || (current_order->trader_id == t->id) || current_order->fulfilled) {
            private_enqueue(dup_book, current_order);
            continue;
        }
        if (current_order->price >= new_order->price) {
            if (current_order->quantity > new_order->quantity) {
                current_order->quantity = current_order->quantity - new_order->quantity;
                process_order_for_sell(current_order, new_order, available_products, fees, fill_message, signal_traders);
                new_order->fulfilled = 1;
                decrement_level(available_products, new_order);
                private_enqueue(dup_book, new_order);
                private_enqueue(dup_book, current_order);
                break;
            } else if (current_order->quantity < new_order->quantity) {
                if (current_order->num_of_orders > 1) {
                    for (struct order *same_order = current_order; same_order != NULL; same_order = same_order->next) {
                        process_order_for_sell(same_order, new_order, available_products, fees, fill_message, signal_traders);
                        new_order->quantity -= same_order->quantity;
                        same_order->fulfilled = 1;
                        if (same_order->next == NULL) {
                            decrement_level(available_products, same_order);
                            private_enqueue(dup_book, current_order);
                            break;
                        }
                        if (new_order->quantity <= 0)
                        {
                            new_order->fulfilled = 1;
                            decrement_level(available_products, new_order);
                            private_enqueue(dup_book, new_order);
                            private_enqueue(dup_book, current_order);
                            break;
                        }
                    }
                } else {
                    process_order_for_sell(current_order, new_order, available_products, fees, fill_message, signal_traders);
                    current_order->fulfilled = 1;
                    new_order->quantity = new_order->quantity - current_order->quantity;
                    private_enqueue(dup_book, current_order);
                    decrement_level(available_products, current_order);
                }
            } else if (current_order->quantity == new_order->quantity) {
                process_order_for_sell(current_order, new_order, available_products, fees, fill_message, signal_traders);
                current_order->fulfilled = 1;
                new_order->fulfilled = 1;
                private_enqueue(dup_book, new_order);
                private_enqueue(dup_book, current_order);
                break;
            }
        } else {
            private_enqueue(dup_book, current_order);
        }
    }
    while(!is_empty(dup_book)) {
        struct order * o = dequeue(dup_book);
        private_enqueue(book, o);
    }
    free_orderbook(dup_book);
}

void process_order_for_buy(struct order* current_order, struct order* new_order, struct products* available_products, int *fees, write_fill fill_message, send_sig signal_traders)
{
    int value = new_order->quantity * current_order->price;
    int fee = roundl(value * (FEE_PERCENTAGE * 0.01));
    *fees += fee;
    log_match_order_to_stdout(BUY, current_order, new_order, new_order->quantity, value, fee, available_products);
    if (current_order->trader->active_status) {
        fill_message(current_order->trader->exchange_fd, current_order->order_id, new_order->quantity);
        signal_traders(current_order->trader->trader_pid);
    }
    if (new_order->trader->active_status) {
        fill_message(new_order->trader->exchange_fd, new_order->order_id, new_order->quantity);
        signal_traders(current_order->trader->trader_pid);
    }
}

void process_buy_order(struct order *new_order, struct order_book *book, struct trader *t, struct products *available_products, 
    write_fill fill_message, send_sig signal_traders, int *fees) {
        for (int i = book->size; i > 0; i--) {
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
                    if (current_order != NULL) {
                        while (current_order->num_of_orders > 1) {
                            process_order_for_buy(current_order, new_order, available_products, fees, fill_message, signal_traders);
                            current_order->fulfilled = 1;
                            new_order->quantity -= current_order->quantity;
                            if (current_order->next == NULL) {
                                break;
                            }
                            struct order *temp = current_order;
                            current_order = current_order->next;
                            free(temp->product_name);
                            free(temp->order_type);
                            free(temp);
                            if (new_order->quantity <= 0) break;
                        }
                    }
                    else {
                        process_order_for_buy(current_order, new_order, available_products, fees, fill_message, signal_traders);
                        new_order->fulfilled = 1;
                        current_order->quantity -= new_order->quantity;
                    }
                } else if (current_order->quantity == new_order->quantity) {
                    process_order_for_buy(current_order, new_order, available_products, fees, fill_message, signal_traders);
                    new_order->fulfilled = 1;
                    current_order->fulfilled = 1;
                }
            }
            
        }
}

void free_orderbook(struct order_book* book)
{
    while (!is_empty(book))
    {
        struct order *o = dequeue(book);
        if (o == NULL) continue;
        if (o->next != NULL) {
            struct order *o1 = o->next;
            while (o1 != NULL) {
                struct order *temp = o1;
                if (o1->next == NULL) {
                    free(o1->product_name);
                    free(o1->order_type);
                    free(o1);
                    break;
                }
                o1 = o1->next;
                free(temp->product_name);
                free(temp->order_type);
                free(temp);
            }
        }
        free(o->product_name);
        free(o->order_type);
        free(o);
    }
    free(book->orders);
    free(book);
}