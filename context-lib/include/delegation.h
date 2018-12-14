#ifndef __DELEGATION_H__
#define __DELEGATION_H__

#include <stdlib.h>
#include "common.h"
#include "map.h"

struct context
{
    char *id;
};

struct delegator
{
    char *id;
    int  ctx;
};

typedef map_t(struct delegator) del_map_t;
typedef map_t(struct context) ctx_map_t;

struct {
    del_map_t m;
} del_tbl;

struct {
    map_int_t m;
} thread_tbl;

struct {
    ctx_map_t m;
} ctx_tbl;

void init_lctx();

struct delegator *_get_del(int del_id);
void update_thread_ctx(int tid, int ctx_id);
void add_del(int del_id);
void add_ctx(int ctx_id);
struct context *get_thread_ctx(int tid);
void instrument_indicator(int c_id);

struct context *_get_ctx(int ctx_id);
struct delegator *_get_del(int del_id);

#endif
