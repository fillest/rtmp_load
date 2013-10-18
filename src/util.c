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

int handle_av_lock (void **mutex, enum AVLockOp op) {
    switch (op) {
       case AV_LOCK_CREATE:
            // fprintf(stderr, "AV_LOCK_CREATE\n");
            *mutex = malloc(sizeof(pthread_mutex_t));
            assert(*mutex != NULL);
            pthread_mutex_init(*mutex, NULL);
            return 0;
       case AV_LOCK_OBTAIN:
            // fprintf(stderr, "AV_LOCK_OBTAIN\n");
            pthread_mutex_lock(*mutex);
            return 0;
       case AV_LOCK_RELEASE:
            // fprintf(stderr, "AV_LOCK_RELEASE\n");
            pthread_mutex_unlock(*mutex);
            return 0;
       case AV_LOCK_DESTROY:
            // fprintf(stderr, "AV_LOCK_DESTROY\n");
            pthread_mutex_destroy(*mutex);
            free(*mutex);
            return 0;
        default:
            return 1;
    }
}