#pragma once


typedef struct {
    int id;
    int delay_ms;
    int is_live;
    // char *rtmp_string;
    char rtmp_string[60];
    // void* rtmp_string;
} Thread_param;

void *process_stream (void *param);