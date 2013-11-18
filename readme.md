# rtmp_load — a load testing tool for RTMP servers

Rough performance numbers (10k threads on one host): CPU 20-50% of each core (uses all cores; Xeon E5-2640, Debian 6 3.2.0-0.bpo), 4-6 Gbit network usage (some SD streams), 14GB RAM


## Setup
Tested on Ubuntu 12 and Debian 6
```bash
sudo apt-get install librtmp0 librtmp-dev yasm pkg-config
sudo apt-get install python-virtualenv
cd rtmp_load
virtualenv --no-site-packages venv
source venv/bin/activate
pip install -r requirements.txt
bold
```


## Usage
Test scenarios are Lua scripts. Basically, a script produces settings and passes them to the special function that runs the test.

Copy and edit the test template:
```bash
cp test.lua.example your_test.lua
```

You may configure:
* *rtmp_string* - server address
* *is_live* - 0 (default) or 1
* *delay_ms* - by default random 0-999 - delay before a thread starts

*spawn_phases* is a list of phase settings. Each of phases is used sequentially to spawn threads. *duration* is duration of phase in seconds and *rate* is how many threads are spawned each second.

*scenarios* is a hash of type "function that produces settings -> weight of choice". Weight is used for weighted random choosing. It isn't in percents but it may be more intuitive to use it as percents, for example:
```lua
local scenarios = {
	[scenario1] = 20,
	[scenario2] = 80,
}
```

Run the test:
```bash
./run.py your_test.lua
```


## Design
There are three layers:
* The C core that handles RTMP stream with [libav](http://libav.org/), [librtmp](http://rtmpdump.mplayerhq.hu/librtmp.3.html) and threads. It collects some metrics and writes it to stdout.
* The Lua/LuaJIT scripting layer that manages threads and configuration
* The Python script that launches the core, consumes the metrics from it and computes statistics


## Issues
Please submit any bugs or feedback to [the issue tracker](https://github.com/fillest/rtmp_load/issues)


## Development
```bash
pip install -r requirements_dev.txt
```

### Updating libav
Remove "version.h" from .gitignore becaused this file is needed for building

Libav 9.9 works bad - "unknown error" from avformat_open_input, strange timings (lots of frame underruns which doesn't happen on 0.8.8)
Though it seems like url_alloc_for_protocol(avformat_open_input) memleaks in 0.8.8 (0.8.9 seems ok?)


## License
See licence.txt ([The MIT License](http://www.opensource.org/licenses/mit-license.php))


## TODO
* try ffmpeg again
