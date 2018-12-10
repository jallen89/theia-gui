#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "delegation.h"

int main() {
    init_lctx();
    struct delegator *del;

    add_del(1);
    del = _get_del(1);
    if (del)
        printf("Received delegator with id %s\n", del->id);
    del = _get_del(2);

    update_thread_ctx(1, 1);
    update_thread_ctx(1, 2);
}
