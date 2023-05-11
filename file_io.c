#include "file_io.h"

#define LOG_PREFIX "[PEX]"

struct products* read_exchaning_prodcuts_from_file(char *filename)
{
    FILE *f = fopen(filename, "r");
    if (f == NULL)
    {
        exit(EXIT_FAILURE);
    }
    struct products *exchaning_products = (struct products*)malloc(sizeof(struct products));
    char num[4];
    fgets(num, 4, f);
    int num_of_products = atoi(num);
    //TODO: check for error
    exchaning_products->num_of_products = num_of_products;
    exchaning_products->items = (char**)malloc(sizeof(char*) * num_of_products);
    for (int i = 0; i < num_of_products; i++) 
    {
        char buffer[128];
		fgets(buffer, 128, f);
		buffer[strlen(buffer) - 1] = '\0';
		int p_len = strlen(buffer);
        exchaning_products->items[i] = (char*)malloc(sizeof(char*) * p_len + 1);
        strncpy(exchaning_products->items[i], buffer, p_len + 1);
		memset(buffer, 0, 128);
    }
    return exchaning_products;
}
void free_products(struct products* ex_products)
{
    for (int i = 0; i < ex_products->num_of_products; i++)
    {
        free(ex_products->items[i]);
    }
    free(ex_products->items);
    free(ex_products);
}
void print_products(struct products *ex_products)
{
    printf("%s Starting\n", LOG_PREFIX);
    printf("%s Trading %d products: ",LOG_PREFIX,  ex_products->num_of_products);
    for (int i = 0; i < ex_products->num_of_products; i++) 
    {
        printf("%s ", ex_products->items[i]);
    }
    printf("\n");
}