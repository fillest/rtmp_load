#include "util.h"


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