local ffi = assert(require 'ffi')
local C = ffi.C


-- ffi.cdef([[
-- 	void *malloc(size_t size);
-- 	char * strcpy ( char * destination, const char * source );
-- 	typedef   void* (*tfn)  ( void *);
-- ]])

local _M = {}


-- package.path = package.path .. ';build/dev/luajit_src/src/?.lua'
-- assert(require("jit.v")).start()
-- assert(require("jit.dump")).start()


function _M.spawn_threads (spawn_phases, scenarios)
	local f = assert(io.open("build/dev/luajit_ffi.h", 'rb'))
	local c = assert(f:read('*a'))
	assert(f:close())
	ffi.cdef(c)

	math.randomseed(os.time())

	local attr = ffi.new("pthread_attr_t[1]")
	C.pthread_attr_init(attr)
	C.pthread_attr_setdetachstate(attr, C.PTHREAD_CREATE_JOINABLE)
	C.pthread_attr_setstacksize(attr, 4194304)
	--1048576

	--print stdout time "starting"

	local threads = {}
	local thread_id = 0
	for _, phase in ipairs(spawn_phases) do
		for _ = 1, phase.duration do
			for _ = 1, phase.rate do
				local params = _M.weighted_random_choice(scenarios)(thread_id)
				assert(params.rtmp_string)

				-- io.stderr:write "a\n"
				local p = ffi.new("Thread_param[1]")
				p[0].id = thread_id
				p[0].delay_ms = params.delay_ms or math.random(0, 999)
				p[0].is_live = params.is_live or 0
				-- local s = ffi.new("char[?]", #params.rtmp_string + 1, params.rtmp_string)
				-- local s = ffi.cast('char*', C.malloc(#params.rtmp_string + 1))
				local s = ffi.new("char[60]")
				-- local s = ffi.new("char[60]", #params.rtmp_string + 1)
				ffi.copy(s, params.rtmp_string)
				-- ffi.copy(s, params.rtmp_string, #params.rtmp_string)
				-- C.strcpy(s, params.rtmp_string)
				p[0].rtmp_string = s
				-- p[0].rtmp_string= ffi.cast('void*', s)
				-- io.stderr:write "b\n"

				local thread = ffi.new("pthread_t[1]")
				-- print("spawninig thread " .. thread_id)
				-- assert(C.pthread_create(thread, attr, C.process_stream, p) == 0)
				-- C.pthread_create(thread, attr, C.process_stream, p)
				C.pthread_create(thread, attr, C.process_stream, p) --TODO NYI: unsupported C type conversion
				-- C.pthread_create(thread, attr, ffi.cast('tfn', C.process_stream), ffi.cast('void*', p)) --TODO NYI: unsupported C type conversion
				-- assert(C.pthread_create(thread, attr, C.process_stream, ffi.cast('void*', p)) == 0)
				-- io.stderr:write "qwe\n"

				table.insert(threads, thread)
				thread_id = thread_id + 1
			end

			_M.nanosleep(1)
		end
	end

	-- io.stderr:write("All threads have been spawned, joining\n")
	for _, thread in ipairs(threads) do
		local status = ffi.new("void *[1]")
		assert(C.pthread_join(thread[0], status) == 0)
	end

	C.pthread_attr_destroy(attr)
end

function _M.weighted_total (choices)
	local total = 0
	for i = 1, #choices do  -- avoid NYI: FastFunc pairs
		total = total + choices[i][2]
	end
	-- for choice, weight in pairs(choices) do
	-- 	total = total + weight
	-- end
	return total
end

function _M.weighted_random_choice (choices)
	local threshold = math.random(0, _M.weighted_total(choices))
	local last_choice
	-- for choice, weight in pairs(choices) do
	for i = 1, #choices do  -- avoid NYI: FastFunc pairs
		local choice = choices[i][1]
		local weight = choices[i][2]
		threshold = threshold - weight
		if threshold <= 0 then return choice end
		last_choice = choice
	end
	return last_choice
end

function _M.nanosleep (sec, nanosec)
	sec = sec or 0
	nanosec = nanosec or 0
	local t = ffi.new("struct timespec", {tv_sec = sec, tv_nsec = nanosec})
	assert(C.nanosleep(t, nil) == 0)
end

function _M.random_seed_with_urandom ()
	local f = assert(io.popen('od -vAn -N4 -tu4 < /dev/urandom', 'r'))
	local seed = tonumber(assert(f:read('*a')))
	f:close()
	math.randomseed(seed)
end

function _M.timestamp_now ()
	--allows JIT to compile (NYI: FastFunc os.time)
	return tonumber(C.time(nil))
end


return _M