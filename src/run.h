#pragma once


typedef struct {
    char *rtmp_string;
    int id;
    int delay_ms;
    int is_live;
} Thread_param;

void *process_stream (void *param);