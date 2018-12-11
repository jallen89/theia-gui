#include "delegation.h"


void init_lctx()
{
    T_DEBUG("Initializing lctx!\n");
    map_init(&del_tbl.m);
    map_init(&thread_tbl.m);
    map_init(&ctx_tbl.m);
}



void add_del(int del_id)
{
    struct delegator del;
    int err;

    del.id = malloc(sizeof(char)*17);
    sprintf(del.id, "%d", del_id);

    T_DEBUG("Adding del_id %s into delegator table.\n", del.id);
    err = map_set(&del_tbl.m, del.id, del);
    if (err)
        T_DEBUG("Failed to insert delegator: %d into del_table\n", del_id);
}


void add_ctx(int ctx_id)
{
    struct context ctx;
    int err;

    ctx.id = malloc(sizeof(char)*17);
    sprintf(ctx.id, "%d", ctx_id);

    T_DEBUG("Adding ctx_id %s into ctx table.\n", ctx.id);
    err = map_set(&ctx_tbl.m, ctx.id, ctx);
    if (err)
        T_DEBUG("Failed to insert ctx: %d into ctx_table\n", ctx_id);
}


void update_thread_ctx(int tid, int del_id)
{
    int err;
    char tmp[17];

    // XXX. Called for debugging.
    // TODO. We should fail here probably.
    _get_del(del_id);
        
    // Insert tid and del_id into thread_tbl.
    T_DEBUG("Adding (%d, %d) into del_table.\n", tid, del_id);
    sprintf(tmp, "%d", tid);
    err = map_set(&thread_tbl.m, tmp, del_id);
    if (err)
        fail("Failed to insert (%d, %d) into del_table.\n", tid, del_id);
}


struct delegator *_get_del(int del_id) {
    struct delegator *del;
    char tmp[17];

    sprintf(tmp, "%d", del_id);
    del = map_get(&del_tbl.m, tmp);
    if (!del)
        T_DEBUG("A delegator with del_id %d does not exist!\n", del_id);

    return del;
}

struct context *_get_ctx(int ctx_id) {
    struct context *ctx;
    char tmp[17];
    sprintf(tmp, "%d", ctx_id);

    ctx = map_get(&ctx_tbl.m, tmp);
    if (!ctx)
        T_DEBUG("A ctx with ctx_id %d does not exist!\n", ctx_id);

    return ctx;
}

