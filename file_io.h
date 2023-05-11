#include "pe_common.h"
#include "pe_exchange.h"

struct products* read_exchaning_prodcuts_from_file(char *);
void free_products(struct products* ex_products);
void print_products(struct products *ex_products);
