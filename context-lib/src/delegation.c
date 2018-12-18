#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>
#include "delegation.h"
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>

FILE *fptr;
int is_initialized = 0;


void init_lctx()
{
  if(is_initialized)
    return;

  is_initialized = 1;
  T_DEBUG("Initializing lctx!\n");
  map_init(&del_tbl.m);
  map_init(&thread_tbl.m);
  map_init(&ctx_tbl.m);

  fptr = fopen("context.log", "wr");
  if (!fptr)
    fail("Failed to open context log!\n");
}

void write_log(long tid, int c_id) {
  char tmp[256];
  struct timeval tv;
  int size;

  gettimeofday(&tv, NULL);
  memset(&tmp, 0, sizeof(tmp));
  size = sprintf(tmp, "%lu|%d|%lu|%lu\n", tid, c_id, tv.tv_sec, tv.tv_usec);
  fwrite(tmp, sizeof(char), sizeof(tmp), fptr);
  fflush(fptr);
}

void instrument_indicator(int c_id)
{
  long tid;
  struct context *p_ctx = NULL;

  init_lctx();
  
  // Add New context to ctx map.
  add_ctx(c_id);
  tid = syscall(SYS_gettid);
#ifdef CONFIG_DEBUG
  p_ctx = get_thread_ctx(tid);
  if (!p_ctx) {
    T_DEBUG("Thread context was null, assuming first assignment!\n")
  } else {
    T_DEBUG("Context Switch: (%s) ~~> (%d)\n", p_ctx->id, c_id);
  }
#endif
  update_thread_ctx(tid, c_id);
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


void instrument_delegator(int del_id)
{
  long tid;
  int err;
  struct context *t_ctx = NULL;
  struct delegator *del = NULL;

  init_lctx();
  // Get delegator struct or create new one.
  del = _get_del(del_id);
  if (!del) {
    del= malloc(sizeof(struct delegator));
    del->id = malloc(sizeof(char)*17);
    sprintf(del->id, "%d", del_id);
  }

  //Get the current thread's context.
  tid = syscall(SYS_gettid);
  t_ctx = get_thread_ctx(tid);

  // Set delegator's context.
  if (!t_ctx) {
    T_DEBUG("The current thread does not have have a context!\n");
  } else {
    del->ctx_id = t_ctx->id;
  }

  // Add delegator to del table.
  err = map_set(&del_tbl.m, del->id, *del);
  if (err) {
      T_DEBUG("Failed to insert delegator: %d into del_table\n", del_id);
  } else {
    T_DEBUG("Inserted %d  (ctx %s) into del_table\n", 
            del_id, del->ctx_id);
  }
}

void instrument_del_indicator(int ctx_id)
{
  long tid;
  init_lctx();
  T_DEBUG("Instrumenting del indicator: ctx %d!\n", ctx_id);
  //long tid;
  //T_INFO("Calling instrument del indicator!\n");
  tid = syscall(SYS_gettid);
  //T_INFO("Setting thread %d ctx to: %d\n", tid, ctx_id);
  update_thread_ctx(tid, ctx_id);
}


void update_thread_ctx(int tid, int ctx_id)
{
  int err;
  char tmp[17];

  // XXX. Called for debugging.
  // TODO. We should fail here probably.
      
  // Insert tid and ctx_id into thread_tbl.
  T_DEBUG("Adding (%d, %d) into del_table.\n", tid, ctx_id);
  sprintf(tmp, "%d", tid);
  err = map_set(&thread_tbl.m, tmp, ctx_id);
  if (err)
      fail("Failed to insert (%d, %d) into ctx_table.\n", tid, ctx_id);

  write_log(tid, ctx_id);
}

struct context *get_thread_ctx(int tid) {
  struct context *t_ctx = NULL;
  int *ctx_id;
  char tmp[17];

   sprintf(tmp, "%d", tid);
   ctx_id = (int *) map_get(&thread_tbl.m, tmp);

  if (!ctx_id) {
    T_DEBUG("Failed to get thread context!\n");
    t_ctx = NULL;
  } else {
    t_ctx = _get_ctx(*ctx_id);
  }

  return t_ctx;
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

