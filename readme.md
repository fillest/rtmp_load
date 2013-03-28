# rtmp_load — a load testing tool for RTMP servers

## Setup
```bash
sudo apt-get install librtmp0 librtmp-dev yasm pkg-config
cd rtmp_load
virtualenv --no-site-packages venv
source venv/bin/activate
pip install bold
bold
```

## Usage
Test scenarios are Lua scripts. Basically, a script produces settings and passes them to the special function that runs the test.

Copy and edit the test template:
```bash
cp test.lua.example your_test.lua
```

You may customize:
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

## Issues
Please submit any bugs or feedback to [the issue tracker](https://github.com/fillest/rtmp_load/issues)

## License
See licence.txt ([The MIT License](http://www.opensource.org/licenses/mit-license.php))