local _M = {}

local ffi = assert(require 'ffi')
local C = ffi.C


function _M.spawn_threads (spawn_phases, scenarios)
	local f = assert(io.open("build/dev/luajit_ffi.h", 'rb'))
	local c = assert(f:read('*a'))
	assert(f:close())
	ffi.cdef(c)

	math.randomseed(os.time())

	local attr = ffi.new("pthread_attr_t[1]")
	C.pthread_attr_init(attr)
	C.pthread_attr_setdetachstate(attr, C.PTHREAD_CREATE_JOINABLE)

	--print stdout time "starting"

	local threads = {}
	local thread_id = 0
	for _, phase in ipairs(spawn_phases) do
		for _ = 1, phase.duration do
			for _ = 1, phase.rate do
				local params = _M.weighted_random_choice(scenarios)(thread_id)
				assert(params.rtmp_string)

				local p = ffi.new("Thread_param")
				p.id = thread_id
				p.delay_ms = params.delay_ms or math.random(0, 999)
				p.is_live = params.is_live or 0
				p.rtmp_string = ffi.new("char[?]", #params.rtmp_string)
				ffi.copy(p.rtmp_string, params.rtmp_string)

				local thread = ffi.new("pthread_t[1]")
				-- print("spawninig thread " .. thread_id)
				assert(C.pthread_create(thread, attr, C.process_stream, p) == 0)

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
	for choice, weight in pairs(choices) do
		total = total + weight
	end
	return total
end

function _M.weighted_random_choice (choices)
	local threshold = math.random(0, _M.weighted_total(choices))
	local last_choice
	for choice, weight in pairs(choices) do
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