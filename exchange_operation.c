#include "pe_common.h"
#define LOG_PREFIX "[PEX]"
int compare_orders(struct order* o1, struct order *o2)
{
    return o1->price < o2->price;
}

struct order_book* create_orderbook(int order_size)
{
    struct order_book *book = (struct order_book*)calloc(1, sizeof(struct order_book));
    book->compare_orders = compare_orders;
    book->capaity = order_size;
    book->orders = (struct order*)(calloc(book->capaity, sizeof(struct order) * book->capaity));
    return book;
}

void free_orderbook(struct order_book* book)
{
    // for (int i = 0; i <= book->capaity; i++) {
    //     free(book->orders[i]);
    // }
    free(book->orders);
    free(book);
}

void swap_orders(struct order_book *book, int i, int j)
{
    struct order temp = book->orders[i];
    book->orders[i] = book->orders[j];
    book->orders[j] = temp;
}
int is_empty(struct order_book *book) {
    return book->size == 0;
}

//bootom-up heapify
void swim(int k, struct order_book *book)
{
    while (k > 1 && book->compare_orders(&book->orders[k/2], &book->orders[k]))
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
        if (j < book->size && book->compare_orders(&book->orders[j], &book->orders[j+1])) j++;
        if (!book->compare_orders(&book->orders[k], &book->orders[j])) break;
        swap_orders(book, k, j);
        k = j;
    }
}

struct order dequeue(struct order_book* book)
{
    struct order temp = book->orders[1];
    swap_orders(book, 1, book->size--);
    sink(1, book);
    return temp;
}
void enqueue_order(struct order_book *book, char * order_type, int order_id, char *product_name, int quantity, int price, int trader_id)
{
    struct order o;
    o.order_type = strcmp("BUY", order_type) == 0 ? "BUY": "SELL";
    o.order_id = order_id;
    o.product_name = product_name;
    o.quantity = quantity;
    o.num_of_orders = 1;
    o.price = price;
    o.trader_id = trader_id;
    if (strcmp(o.order_type, "BUY") == 0) book->buy_level += 1;
    if (strcmp(o.order_type, "SELL") == 0) book->sell_level += 1;
    if (book->size == book->capaity) 
    {
        book->capaity *= 2;
        book->orders = (struct order*)realloc(book->orders, sizeof(struct order) * book->capaity);
    }
    if (book->size == 0)
    {
        book->orders[++book->size] = o;
        // memcpy(book->orders[++book->size], &o, sizeof(struct order));
    }
    else
    {
        int o_size = book->size;
        int found = 0;
        int found_index;
        while (!is_empty(book))
        {
            struct order o1 = dequeue(book);
            if (o.price == o1.price && strcmp(o.product_name, o1.product_name) == 0)
            {
                found = 1;
                found_index = book->size;
            }
        }
        book->size = o_size;
        swim(book->size, book);
        if (found)
        {
            book->orders[found_index].quantity += o.quantity;
            book->orders[found_index].num_of_orders += 1;
            book->buy_level -= 1;
            
        } 
        else {
            book->orders[++book->size] = o;
            // memcpy(book->orders[++book->size], &o, sizeof(struct order));
            swim(book->size, book);    
        }
        
    }
}

int search_product_in_book(char *product_name, struct order_book* book) {
    for (int i = 1; i <= book->size; i++) {
        char *p_name = book->orders[i].product_name;
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
        char *p_name = available_products->items[i];
        int product_found = search_product_in_book(p_name, book);
        if (!product_found) {
            printf("%s\tProduct: %s; Buy level: %d; Sell levels: %d\n", LOG_PREFIX, available_products->items[i], 0, 0);
        } else {
            printf("%s\tProduct: %s; Buy level: %d; Sell levels: %d\n", LOG_PREFIX, p_name, book->buy_level, book->sell_level);
            int original_size = book->size;
            while (!is_empty(book))
            {
                struct order o = dequeue(book);
                    if (o.num_of_orders > 1) {
                        printf("%s\t\t%s %d @ %d (%d orders)\n",LOG_PREFIX, o.order_type, o.quantity, o.price, o.num_of_orders);    
                    } else {
                        printf("%s\t\t%s %d @ %d (%d order)\n", LOG_PREFIX, o.order_type, o.quantity, o.price, o.num_of_orders);
                    }
            }
            book->size = original_size;
            swim(book->size, book);
        }
    }
}

void print_position(struct products * ex_products, struct trader **traders, int num_of_traders) {
    printf("%s\t--POSITIONS--\n", LOG_PREFIX);
    for (int j = 0; j < num_of_traders; j++) {
        printf("%s\tTrader %d: ",LOG_PREFIX, traders[j]->trader_fifo_id);
        for (int k = 0; k < ex_products->num_of_products; k++) {
            if (k == ex_products->num_of_products - 1) {
                printf("%s %d ($%d)\n", ex_products->items[k], traders[j]->position_qty[k], traders[j]->position_price[k]);    
            } else {
                printf("%s %d ($%d), ", ex_products->items[k], traders[j]->position_qty[k], traders[j]->position_price[k]);
            }
        }
    }
}