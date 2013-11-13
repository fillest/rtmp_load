#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <librtmp/rtmp.h>
#include <librtmp/log.h>
#include "gopt.h"
#include "util.h"
#include "run.h"


typedef struct timespec timespec_;

typedef struct {
    pthread_mutex_t mutex;
    int frame_num;
    bool should_stop;
} counter_thread_param_t;


#define ERRBUF_SZ 1024
char errbuf[ERRBUF_SZ];
#define FPS 25


// __thread bool got_timestamp = false;
__thread timespec_ connected_time;

// pthread_mutex_t mt;


timespec_ get_time () {
    timespec_ time;
    assert(clock_gettime(CLOCK_MONOTONIC, &time) == 0);
    return time;
}

void *track_fps (void *args_) {
    counter_thread_param_t* args = (counter_thread_param_t*) args_;

    timespec_ second;
    second.tv_sec = 1;
    second.tv_nsec = 0;
    while (true) {
        pthread_mutex_lock(&(args->mutex));
        int frame_num = args->frame_num;
        bool should_stop = args->should_stop;
        pthread_mutex_unlock(&(args->mutex));
        // fprinttfn(stdout, "@buf_frame_num %i", frame_num);

        if (should_stop) {
            fprintf(stderr, "note: stopping track_fps thread\n");
            pthread_exit(NULL);
            assert(0);
        }

        assert(! nanosleep(&second, NULL));

        pthread_mutex_lock(&(args->mutex));
        args->frame_num -= FPS;
        pthread_mutex_unlock(&(args->mutex));
    }

    pthread_exit(NULL);
}

int get_stream_index (AVFormatContext *av_context) {
    int videoStream = -1;
    for (int i = 0; i < av_context->nb_streams; i++) {
        if (av_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            assert(videoStream == -1);
            videoStream = i;
        }
    }
    if (videoStream == -1) {
        fprintf(stderr, "failed to find video stream\n");
        exit(EXIT_FAILURE);
    }
    return videoStream;
}

void *process_stream (void *args_) {
    // puts("_");
    Thread_param *params = (Thread_param *) args_;

    if (params->delay_ms) {
        assert(params->delay_ms >= 0 && params->delay_ms <= 999);
        // fprinttfn(stderr, "gonna sleep %ims", params->delay_ms);
        timespec_ sleep_time;
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = params->delay_ms * 1000000;
        assert(! nanosleep(&sleep_time, NULL));
    }

    // fprinttfn(stdout, "@starting_thread %i", params->id);

    // timespec_ start_time;
    // assert(clock_gettime(CLOCK_MONOTONIC, &start_time) == 0);

    AVFormatContext *av_context = NULL;
    // puts("((");
    // fprintf(stderr, "str: %s\n", params->rtmp_string);
    // puts("))");
    // pthread_mutex_lock(&mt);
    // puts("1");
    int err = avformat_open_input(&av_context, params->rtmp_string, NULL, NULL); 
    // puts("2");
    // pthread_mutex_unlock(&mt);
    if (err != 0) {
        av_strerror(err, errbuf, ERRBUF_SZ);
        // fprintf(stderr, "avformat_open_input() failed: %s\n", errbuf);
        fprinttfn(stdout, "@error avformat_open_input() fail: %s", errbuf);
        // exit(EXIT_FAILURE);

        //TODO review
        fprinttfn(stdout, "@stopping_thread failed to open stream");
        // avformat_close_input(&av_context);
        pthread_exit(NULL);
    }

    //***NetStream.Timestamp shows in log here already

    //*** it reads several frames
    // pthread_mutex_lock(&mt);
    err = avformat_find_stream_info(av_context, NULL);
    // pthread_mutex_unlock(&mt);
    if (err < 0) {
        av_strerror(err, errbuf, ERRBUF_SZ);
        fprintf(stderr, "avformat_find_stream_info() failed: %s\n", errbuf);
        exit(EXIT_FAILURE);
    }

    /* Dump information about file onto standard error */
    // av_dump_format(av_context, 0, "t.flv", false);

    int videoStream = get_stream_index(av_context);


    // int buffered_frame_num = 0;
    bool is_first_frame = true;
    counter_thread_param_t cparam;

    AVPacket packet;
    av_init_packet(&packet);

    // timespec_ start_time;
    // assert(clock_gettime(CLOCK_MONOTONIC, &start_time) == 0);

    /*
    So we read the frames, accumulate the numbers and check passed time. When N seconds has passed, we subtract
    FPS * N from accumulated frame number implying a "player" consumes FPS and there should be >= FPS frames in buffer each second
    */

    

    //"For video, the packet contains exactly one frame"
    //"This function returns what is stored in the file, and does not validate that what is there are valid frames for the decoder.
    //It will split what is stored in the file into frames and return one for each call.
    //It will not omit invalid data between valid frames so as to give the decoder the maximum information possible for decoding"
    while ((err = av_read_frame(av_context, &packet)) == 0) {
        if (packet.stream_index == videoStream) {
            if (is_first_frame) {
                is_first_frame = false;

                timespec_ first_frame_time = get_time();
                timespec_ first_frame_latency = subtract_timespec(first_frame_time, connected_time);
                // fprinttfn(stdout, "@first_frame %ld %ld", first_frame_latency.tv_sec, first_frame_latency.tv_nsec);

                // if (params->is_live && (! got_timestamp)) {
                //     fprinttfn(stdout, "@error not_live");
                // }


                pthread_t thr;
                pthread_mutex_init(&(cparam.mutex), NULL);
                cparam.frame_num = 0;
                cparam.should_stop = false;
                pthread_attr_t attributes;
                pthread_attr_init(&attributes);
                pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
                int r = pthread_create(&thr, &attributes, track_fps, (void *) &cparam);
                if (r) {
                    fprintf(stderr, "failed to create track_fps thread: %i\n", r);
                    exit(EXIT_FAILURE);
                }
                assert(pthread_attr_destroy(&attributes) == 0);
            }

            // timespec_ cur_time;
            // assert(clock_gettime(CLOCK_MONOTONIC, &cur_time) == 0);
            // timespec_ time_passed = subtract_timespec(cur_time, start_time);

            // fprinttfn(stdout, "@frame");
            
            pthread_mutex_lock(&(cparam.mutex));
            cparam.frame_num++;
            pthread_mutex_unlock(&(cparam.mutex));
            // buffered_frame_num++;

            // if (time_passed.tv_sec > 0) {
            //     buffered_frame_num -= time_passed.tv_sec * FPS;
            //     //TODO !!! time_passed.tv_nsec

            //     // fprinttfn(stdout, "@buffered_frame_num %i", buffered_frame_num);

            //     start_time = cur_time;

            //     // if (buffered_frame_num < 0) {
            //     //     fprinttfn(stdout, "@underrun %i", buffered_frame_num);
            //     // }
            // }
        }

        av_free_packet(&packet);
        // av_init_packet(&packet); //? -- there's no call in http://libav.org/doxygen/master/pktdumper_8c_source.html but there is in some snippets
    }

    av_strerror(err, errbuf, ERRBUF_SZ);

    pthread_mutex_lock(&(cparam.mutex));
    cparam.should_stop = true;
    pthread_mutex_unlock(&(cparam.mutex));

    fprinttfn(stdout, "@stopping_thread %s", errbuf);  //timeout in rtmp string leads to "End of file"

    avformat_close_input(&av_context);

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
        // fputs(fmt, stderr);
        // fputs(line, stderr);

        if (strstr(line, "NetConnection.Connect.Success") != NULL) {
            assert(clock_gettime(CLOCK_MONOTONIC, &connected_time) == 0);
        }

        // double i1, i2;
        // int n = sscanf(line, "Property: <Name: description, STRING: %lf, %lf, %*d, %*d", &i1, &i2);
        // if (n == 2) {
        //     got_timestamp = true;
        //     // printf("%f, %f, %f\n", i1, i2, i1 - i2);
        //     fprinttfn(stdout, "@diff %f", i1 - i2);
        // }
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
    lua_gc(lua_state, LUA_GCSTOP, 0);
    luaL_openlibs(lua_state);
    lua_gc(lua_state, LUA_GCRESTART, -1);
    int r = luaL_loadfile(lua_state, script_path);
    if (r != 0) {
        if (r == LUA_ERRSYNTAX) {
            fprintf(stderr, "error loading %s, syntax error: %s\n", script_path, lua_tostring(lua_state, -1));
        } else if (r == LUA_ERRFILE) {
            fprintf(stderr, "failed to open %s: %s\n", script_path, lua_tostring(lua_state, -1));
        } else {
            fprintf(stderr, "error loading %s, ret = %i\n", script_path, r);
        }
        exit(EXIT_FAILURE);
    }
    lua_call(lua_state, 0, 1);


    // av_log_set_level(AV_LOG_ERROR);
    // av_log_set_level(AV_LOG_INFO); //http://libav.org/doxygen/master/log_8h.html
    av_log_set_level(AV_LOG_VERBOSE); // for parsing lib log
    // av_log_set_level(AV_LOG_DEBUG); // shows binary packet data
    

    //TODO http://libav.org/doxygen/master/librtmp_8c_source.html#l00080

    av_log_set_callback(av_log_my_callback);

    av_register_all();
    avcodec_register_all();
    avformat_network_init();

    if (av_lockmgr_register(handle_av_lock) != 0) {
        fprintf(stderr, "av_lockmgr_register failed\n");
        exit(EXIT_FAILURE);
    }

    lua_call(lua_state, 0, 0);


    lua_close(lua_state);

    av_lockmgr_register(NULL);
    avformat_network_deinit();


    exit(EXIT_SUCCESS);
    return 0;
}
