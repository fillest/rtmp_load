#pragma once

#include <pthread.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "libavformat/avformat.h"
// #include "libavutil/common.h"
// #include "libavutil/avutil.h"
#include <assert.h>
#include <time.h>


void lua_stack_dump (lua_State *L);
struct timespec diff_ts (struct timespec start, struct timespec end);
int handle_av_lock (void **mtx, enum AVLockOp op);


    /* int milli = curTime.tv_nsec / 1000000; \ */
#define fprinttfn(stream, format, ...) do { \
    struct timespec curTime; \
    assert(clock_gettime(CLOCK_REALTIME, &curTime) == 0); \
    int micro = curTime.tv_nsec / 1000; \
\
    struct tm bdt; \
    char buffer [30]; \
    strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", gmtime_r(&curTime.tv_sec, &bdt)); \
\
    fprintf(stream, "%s.%d " format "\n", buffer, micro, ##__VA_ARGS__); \
} while (0)
