#include "util.h"


// void fprinttfn (FILE *stream, const char *format, ...) {
//     struct timespec curTime;
//     assert(clock_gettime(CLOCK_REALTIME, &curTime) == 0);

//     // struct timeval curTime;
//     // gettimeofday(&curTime, NULL);
//     // int milli = curTime.tv_usec / 1000;
//     int milli = curTime.tv_nsec / 1000000;

//     struct tm bdt;
//     char buffer [30];
//     strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", gmtime_r(&curTime.tv_sec, &bdt));

//     char currentTime[40] = "";
//     sprintf(currentTime, "%s.%d", buffer, milli);

//     char b1[100] = "";
//     va_list ap;
//     va_start(ap, format);
//     vsprintf(b1, format, ap);
//     va_end(ap);

//     fprintf(stream, "%s %s\n", currentTime, b1);
// }

void lua_stack_dump (lua_State *L) {
    printf("** stack dump:\n");

    int top = lua_gettop(L);
    for (int i = 1; i <= top; i ++) {
        printf("   %2i: ", i);

        int t = lua_type(L, i);
        switch (t) {
            case LUA_TSTRING: {
                printf("'%s'", lua_tostring(L, i));
                break;
            }
            case LUA_TBOOLEAN: {
                printf(lua_toboolean(L, i) ? "true" : "false");
                break;
            }
            case LUA_TNUMBER: {
                printf("%g", lua_tonumber(L, i));
                break;
            }
            default: {
                printf("(%s)", lua_typename(L, t));
                break;
            }
        }

        printf("\n");
    }
}

struct timespec diff_ts (struct timespec start, struct timespec end) {
    //TODO use difftime?
    struct timespec temp;
    if ((end.tv_nsec-start.tv_nsec) < 0) {
        temp.tv_sec = end.tv_sec-start.tv_sec - 1;
        temp.tv_nsec = 1e9 + end.tv_nsec - start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

//http://stackoverflow.com/questions/2999075/generate-a-random-number-within-range/2999130#2999130
//http://www.azillionmonkeys.com/qed/random.html
// #define RAND_INV_RANGE(r) ((int) ((RAND_MAX + 1) / (r)))
// int rand_inv (int from, int to) {

//     // int max = to - from + 1;
//     // int x;
//     // do {
//     //     x = rand();
//     // } while (x >= max * RAND_INV_RANGE(max));
//     // x /= RAND_INV_RANGE(max);
//     // return x + from;

//     return rand() % (to - from + 1) + from;
//     // do {
//     //     x = rand();
//     // } while (x >= to * RAND_INV_RANGE (to));
//     // x /= RAND_INV_RANGE (to);
//     // return x
// }

int handle_av_lock (void **mtx, enum AVLockOp op) {
    switch (op) {
       case AV_LOCK_CREATE:
            *mtx = malloc(sizeof(pthread_mutex_t));
            assert(*mtx != NULL);
            pthread_mutex_init(*mtx, NULL);
            return 0;
       case AV_LOCK_OBTAIN:
            // fprintf(stderr, "lock\n");
            pthread_mutex_lock(*mtx);
            return 0;
       case AV_LOCK_RELEASE:
            // fprintf(stderr, "unlock\n");
            pthread_mutex_unlock(*mtx);
            return 0;
       case AV_LOCK_DESTROY:
            pthread_mutex_destroy(*mtx);
            free(*mtx);
            return 0;
    }
    return 1;
}