#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <librtmp/rtmp.h>
#include <librtmp/log.h>
#include "gopt.h"
#include "util.h"
#include "run.h"


typedef struct timespec _timespec;


#define ERRBUF_SZ 1024
char errbuf[ERRBUF_SZ];


__thread bool got_timestamp = false;
__thread _timespec connected_time;


void *process_stream (void *param) {
    Thread_param *params = (Thread_param *) param;

    if (params->delay_ms) {
        assert(params->delay_ms >= 0 && params->delay_ms <= 999);
        // fprinttfn(stderr, "gonna sleep %ims", params->delay_ms);
        _timespec sleep_time;
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = params->delay_ms * 1000000;
        assert(! nanosleep(&sleep_time, NULL));
    }

    fprinttfn(stdout, "@starting_thread %i", params->id);

    // _timespec start_time;
    // assert(clock_gettime(CLOCK_MONOTONIC_RAW, &start_time) == 0);

    AVFormatContext *avformat_context = NULL;
    // fprintf(stderr, "str: %s\n", params->rtmp_string);
    int err = avformat_open_input(&avformat_context, params->rtmp_string, NULL, NULL); 
    if (err != 0) {
        av_strerror(err, errbuf, ERRBUF_SZ);
        fprintf(stderr, "avformat_open_input() failed: %s\n", errbuf);
        exit(EXIT_FAILURE);
    }

    //***NetStream.Timestamp shows in log here already

    //*** it reads several frames
    err = avformat_find_stream_info(avformat_context, NULL);
    if (err < 0) {
        av_strerror(err, errbuf, ERRBUF_SZ);
        fprintf(stderr, "avformat_find_stream_info() failed: %s\n", errbuf);
        exit(EXIT_FAILURE);
    }

    /* Dump information about file onto standard error */
    // av_dump_format(avformat_context, 0, "t.flv", false);

    int videoStream = -1;
    for (int i = 0; i < avformat_context->nb_streams; i++) {
        // fprintf(stderr, "detected stream %i\n", i);
        if (avformat_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            assert(videoStream == -1);
            videoStream = i;
        }
    }
    if (videoStream == -1) {
        fprintf(stderr, "failed to find video stream\n");
        exit(1);
    }


    int buffered_frame_num = 0;
    bool is_first_frame = true;

    AVPacket packet;
    av_init_packet(&packet);

    _timespec start_time;
    assert(clock_gettime(CLOCK_MONOTONIC_RAW, &start_time) == 0);

    /*
    So we read the frames, accumulate the numbers and check passed time. When N seconds has passed, we subtract
    FPS * N from accumulated frame number implying a "player" consumes FPS and there should be >= FPS frames in buffer each second
    */

    //TODO is there any client rate limiting logic?

    //"Technically a packet can contain partial frames or other bits of data, but ffmpeg's parser ensures that the packets we get contain either complete or multiple frames."
    while ((err = av_read_frame(avformat_context, &packet)) == 0) {
        if (packet.stream_index == videoStream) {
            if (is_first_frame) {
                _timespec first_frame_time;
                assert(clock_gettime(CLOCK_MONOTONIC_RAW, &first_frame_time) == 0);
                _timespec first_frame_latency = subtract_timespec(first_frame_time, connected_time);
                fprinttfn(stdout, "@first_frame %ld %ld", first_frame_latency.tv_sec, first_frame_latency.tv_nsec);

                // if (params->is_live && (! got_timestamp)) {
                //     fprinttfn(stdout, "@error not_live");
                // }
            }

            _timespec cur_time;
            assert(clock_gettime(CLOCK_MONOTONIC_RAW, &cur_time) == 0);
            _timespec time_passed = subtract_timespec(cur_time, start_time);

            is_first_frame = false;
            buffered_frame_num++;

            if (time_passed.tv_sec > 0) {
                buffered_frame_num -= time_passed.tv_sec * 25;
                //TODO !!! time_passed.tv_nsec

                fprinttfn(stdout, "@buffered_frame_num %i", buffered_frame_num);

                start_time = cur_time;

                // if (buffered_frame_num < 0) {
                //     fprinttfn(stdout, "@underrun %i", buffered_frame_num);
                // }
            }

            av_free_packet(&packet);
            //av_init_packet(&packet);? -- there's no call in http://libav.org/doxygen/master/pktdumper_8c_source.html but there is in some snippets
        }
    }

    av_strerror(err, errbuf, ERRBUF_SZ);
    fprinttfn(stdout, "@stopping_thread %s", errbuf);  //timeout in rtmp string leads to "End of file"

    avformat_close_input(&avformat_context);

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
            assert(clock_gettime(CLOCK_MONOTONIC_RAW, &connected_time) == 0);
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

    pthread_exit(NULL);
    // exit(0);
}
