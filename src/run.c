#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <librtmp/rtmp.h>
#include <librtmp/log.h>
#include "gopt.h"
#include "util.h"
#include "run.h"


#define ERRBUF_SZ 1024
char errbuf[ERRBUF_SZ];

// pthread_mutex_t mutexsum;

__thread bool got_timestamp = false;

void *process_stream (void *param) {
    Thread_param *params = (Thread_param *) param;

    if (params->delay_ms) {
        assert(params->delay_ms >= 0 && params->delay_ms <= 999);
        // fprinttfn(stderr, "gonna sleep %ims", params->delay_ms);
        struct timespec st;
        st.tv_sec = 0;
        st.tv_nsec = params->delay_ms * 1000000;
        assert(! nanosleep(&st, NULL));
    }

    fprinttfn(stdout, "@starting_thread");
    //TODO remove id?
    // fprinttfn(stderr, "running thread %i", params->id);
    // pthread_exit(NULL);

    struct timespec time_connecting;
    assert(clock_gettime(CLOCK_MONOTONIC_RAW, &time_connecting) == 0);

    AVFormatContext *fctx = NULL;
    // pthread_mutex_lock(&mutexsum);
    // fprintf(stderr, "str: %s\n", params->rtmp_string);
    int err = avformat_open_input(&fctx, params->rtmp_string, NULL, NULL); 
    // pthread_mutex_unlock(&mutexsum);
    if (err != 0) {
        av_strerror(err, errbuf, ERRBUF_SZ);
        fprintf(stderr, "avformat_open_input() failed: %s\n", errbuf);
        exit(EXIT_FAILURE);
    }

    //***NetStream.Timestamp shows in log here already

    //*** it reads several frames
    // pthread_mutex_lock(&mutexsum);
    err = avformat_find_stream_info(fctx, NULL);
    // pthread_mutex_unlock(&mutexsum);
    if (err < 0) {
        av_strerror(err, errbuf, ERRBUF_SZ);
        fprintf(stderr, "avformat_find_stream_info() failed: %s\n", errbuf);
        exit(EXIT_FAILURE);
    }

    /* Dump information about file onto standard error */
    // av_dump_format(fctx, 0, "t.flv", false);

    int videoStream = -1;
    for (int i = 0; i < fctx->nb_streams; i++) {
        // fprintf(stderr, "detected stream %i\n", i);
        if (fctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            assert(videoStream == -1);
            videoStream = i;
        }
    }
    if (videoStream == -1) {
        fprintf(stderr, "failed to find video stream\n");
        exit(1);
    }

    AVPacket pkt;
    av_init_packet(&pkt);

    struct timespec time_last;
    assert(clock_gettime(CLOCK_MONOTONIC_RAW, &time_last) == 0);

    int buffered_frame_num = 0;
    int total_frame_num  = 0;

    while ((err = av_read_frame(fctx, &pkt)) == 0) { //"Technically a packet can contain partial frames or other bits of data, but ffmpeg's parser ensures that the packets we get contain either complete or multiple frames. "
        if (pkt.stream_index == videoStream) {
            if (! total_frame_num) {
                struct timespec time_first_frame;
                assert(clock_gettime(CLOCK_MONOTONIC_RAW, &time_first_frame) == 0);
                struct timespec d = diff_ts(time_connecting, time_first_frame);
                fprinttfn(stdout, "@first_frame %ld %ld", d.tv_sec, d.tv_nsec); //? tv_sec is time_t..

                if (params->is_live && (! got_timestamp)) {
                    fprinttfn(stdout, "@error not_live");
                }
            }
            struct timespec time_next;
            assert(clock_gettime(CLOCK_MONOTONIC_RAW, &time_next) == 0);
            struct timespec d = diff_ts(time_last, time_next);

            total_frame_num++;

            if (d.tv_sec > 0) {
                fprinttfn(stdout, "@buffered_frame_num %i", buffered_frame_num);
                buffered_frame_num -= d.tv_sec * 25; //TODO maybe not accurate (ms)
                time_last = time_next;

                // if (buffered_frame_num < 0) {
                //     fprinttfn(stdout, "@underrun %i", buffered_frame_num);
                // }
            }

            buffered_frame_num++;

            av_free_packet(&pkt);
            // //av_init_packet(&packet);? -- there's no call in http://libav.org/doxygen/master/pktdumper_8c_source.html but there is in some snippets
        }
    }
    av_strerror(err, errbuf, ERRBUF_SZ);
    fprinttfn(stdout, "@stopping_thread %s", errbuf);  //timeout in rtmp string leads to "End of file"

    avformat_close_input(&fctx);

    pthread_exit(NULL);
}

void av_log_my_callback (void* ptr, int level, const char* fmt, va_list vl) {
    AVClass* avc = ptr ? *(AVClass **) ptr : NULL;
    if (level == AV_LOG_VERBOSE) {
    // if (0) {
        char line[1024];
        line[0] = 0;
        if (avc) {
            if (avc->parent_log_context_offset) {
                AVClass** parent = *(AVClass ***) (((uint8_t *) ptr) + avc->parent_log_context_offset);
                if (parent && *parent) {
                    snprintf(line, sizeof(line), "[%s @ %p] ", (*parent)->item_name(parent), parent);
                }
            }
            snprintf(line + strlen(line), sizeof(line) - strlen(line), "[%s @ %p] ", avc->item_name(ptr), ptr);
        }
        vsnprintf(line + strlen(line), sizeof(line) - strlen(line), fmt, vl);
        // vsnprintf(line + strlen(line), sizeof(line) - strlen(line), fmt, vl);
        // vsnprintf(line, sizeof(line), fmt, vl);
        // fputs(line, stderr);
        // fputs(fmt, stderr);

        double i1, i2;
        int n = sscanf(line, "Property: <Name: description, STRING: %lf, %lf, %*d, %*d", &i1, &i2);
        if (n == 2) {
            got_timestamp = true;
            // printf("%f, %f, %f\n", i1, i2, i1 - i2);
            fprinttfn(stdout, "@diff %f", i1 - i2);
        }
    }
    
    // 
    //     fprintf(stderr, fmt);

    //http://libav.org/doxygen/master/log_8c_source.html#l00158
    // av_log_default_callback(ptr, level, fmt, vl);
}

int main (int argc, char *argv[]) {
    // setvbuf(stdout, NULL, _IONBF, 0);
    setlinebuf(stdout);

    assert(setenv("TZ", "UTC", 1) == 0);
    tzset();

    // see http://www.purposeful.co.uk/software/gopt/
    void *options = gopt_sort(&argc, (const char **)argv, gopt_start(
        gopt_option('h', 0, gopt_shorts('h', '?'), gopt_longs("help", "HELP"))//,
        // gopt_option('t', GOPT_ARG, gopt_shorts('t'), gopt_longs("thread-num")),
        // gopt_option('s', 0, gopt_shorts('s'), gopt_longs("silent", "quiet")),
    ));

    //TODO make customizable
    assert(setenv("LUA_PATH", "src/?.lua", 0) == 0);

    //http://rtmpdump.mplayerhq.hu/librtmp.3.html
    // const char usage[] = "usage: rtmp_load [--thread-num/-t N(default 1)] \"rtmp://example.com:1935/ app=app_name_or_empty playpath=path timeout=30\"\n";
    const char usage[] = "usage: rtmp_load test.lua\n";
    if (gopt(options, 'h')) {
        fprintf(stdout, usage);
        exit(EXIT_SUCCESS);
    }

    // const char *opt_thread_num;
    // int thread_num  = 1;
    // if (gopt_arg(options, 't', &opt_thread_num)) {
    //     thread_num = atoi(opt_thread_num);
    //     if (! thread_num) {
    //         fprintf(stderr, "invalid thread num was passed\n");
    //         exit(EXIT_FAILURE);
    //     }
    // }

    gopt_free(options);

    if (argc != 2) {
        // fprintf(stderr, "provide target rtmp string\n");
        fprintf(stderr, "provide test file\n");
        fprintf(stderr, usage);
        exit(EXIT_FAILURE);
    }

    char *script_path = argv[1];

    lua_State *lua_state = luaL_newstate();
    assert(lua_state != NULL);
    luaL_openlibs(lua_state);
    int r = luaL_loadfile(lua_state, script_path);
    if (r != 0) {
        if (r == LUA_ERRSYNTAX) {
            fprintf(stderr, "error loading %s, syntax error: %s\n", script_path, lua_tostring(lua_state, -1));
        } else {
            fprintf(stderr, "error loading %s, ret = %i\n", script_path, r);
        }
        exit(EXIT_FAILURE);
    }
    lua_call(lua_state, 0, 1);


    // av_log_set_level(AV_LOG_ERROR);
    // av_log_set_level(AV_LOG_INFO); //http://libav.org/doxygen/master/log_8h.html
    av_log_set_level(AV_LOG_VERBOSE); // for rtmp
    // av_log_set_level(AV_LOG_DEBUG);
    

    //TODO http://libav.org/doxygen/master/librtmp_8c_source.html#l00080

    av_log_set_callback(av_log_my_callback);

    av_register_all();
    avcodec_register_all();
    avformat_network_init();

    if (av_lockmgr_register(handle_av_lock) != 0) {
        fprintf(stderr, "av_lockmgr_register failed\n");
        exit(EXIT_FAILURE);
    }


    // fprinttfn(stderr, "spawning %i thread(s)", thread_num);

    // pthread_mutex_init(&mutexsum, NULL);

    // pthread_t threads[9000];
    // pthread_t *threads = (pthread_t *) malloc(sizeof(pthread_t) * thread_num);
    // if (threads == NULL) {
    //     fprintf(stderr, "failed to allocate threads\n");
    //     exit(EXIT_FAILURE);
    // }

    // struct timeval time1;
    // gettimeofday(&time1, NULL);
    // srand48((unsigned int) time1.tv_usec);
    // printf("%f\n", drand48());

    // srand(time(NULL)); //need?

    // fprinttfn(stdout, "starting");

    // pthread_attr_t attr;
    // pthread_attr_init(&attr);
    // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    // pthread_attr_setstacksize (&attr, stacksize);
    // int r;
    // struct timespec st;
    lua_call(lua_state, 0, 0);
    // void *status;
    // pthread_join(threads[0], &status);
    // exit(0);





    // for (int i = 0; i < thread_num; ++i) {
        // Thread_param params = {.rtmp_string = "rtmp://localhost:1935/ app=live playpath=t timeout=30", .id = i};
        // r = pthread_create(&threads[i], &attr, process_stream, &params);
        // if (r) {
        //     printf("return code from pthread_create() is %d\n", r);
        //     exit(EXIT_FAILURE);
        // }

        // if (thread_num > 1) {
        //     st.tv_sec = 0;
        //     st.tv_nsec = rand_inv(0, 999) * 1000000;
        //     assert(! nanosleep(&st, NULL));
        // }
    // }
    // fprinttfn(stderr, "all threads were spawned");
    
    // pthread_attr_destroy(&attr);
    // for (int i = 0; i < thread_num; ++i) {
        // void *status;
        // r = pthread_join(threads[i], &status);
        // if (r) {
            // printf("ERROR; return code from pthread_join() is %d\n", r);
            // exit(EXIT_FAILURE);
         // }
    // }
    // pthread_mutex_destroy(&mutexsum);

    // free((void *) threads);

    lua_close(lua_state);

    av_lockmgr_register(NULL);
    avformat_network_deinit();

    pthread_exit(NULL);
    // exit(0);
}
