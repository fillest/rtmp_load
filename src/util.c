#include "util.h"


struct timespec subtract_timespec (struct timespec finish, struct timespec start) {
    struct timespec result;
    if ((finish.tv_nsec - start.tv_nsec) < 0) {
        result.tv_sec = finish.tv_sec - start.tv_sec - 1;
        result.tv_nsec = (finish.tv_nsec - start.tv_nsec) + 1e9;
    } else {
        result.tv_sec = finish.tv_sec - start.tv_sec;
        result.tv_nsec = finish.tv_nsec - start.tv_nsec;
    }
    return result;
}

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