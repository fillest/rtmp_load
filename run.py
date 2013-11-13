#!/usr/bin/env python
import subprocess
import sys
import signal
import datetime
import argparse
import requests
import calendar
import json
import zlib
import time


def get_median (values):
	sv = sorted(values)
	if len(sv) % 2 == 1:
		return sv[(len(sv) + 1) / 2 - 1]
	else:
		lower = sv[len(sv) / 2 - 1]
		upper = sv[len(sv) / 2]

		return float(lower + upper) / 2.0

def parse_args ():
	parser = argparse.ArgumentParser()
	parser.add_argument('test_name', help = "test file path")
	parser.add_argument('-b', '--executable-path', default = 'build/dev/rtmp_load', help = "rtmp_load executable path")
	parser.add_argument('-sh', '--stat-host', default = '10.40.25.155:7778', help = "stat server host[:port]")
	return parser.parse_args()

def parse_line (line):
	parts = line.strip().split()
	try:
		t = datetime.datetime.strptime(parts[0] + ' ' + parts[1], '%Y-%m-%d %H:%M:%S.%f')
	except:
		print >> sys.stderr, "Invalid line:", line 
		raise
	rtype = parts[2]
	return rtype, t, parts

def mistress_server_start_test (test_name, stat_host):
	with open(test_name, 'rb') as f:
		test_source = f.read()

	resp = requests.post('http://%s/new_test' % stat_host, params = {
		'worker_num': 1,
		'project_id': 'rtmp',
		'delayed_start_time': calendar.timegm(datetime.datetime.utcnow().utctimetuple()),
	}, data = test_source)
	assert resp.status_code == requests.codes.ok
	return int(resp.text)

class stypes (object):
	CONCUR_USERS_NUM_MAX = 2
	START_SESSION = 3
	RESPONSE_TIME = 4
	RESPONSE_STATUS = 5
	REQUEST_SENT = 6
	CONNECT_TIME = 7
	CONCUR_USERS_NUM_MIN = 8
	CONNECT_ERROR = 9
	RESPONSE_ERROR = 10
	CONCUR_CONNS_NUM_MIN = 11
	CONCUR_CONNS_NUM_MAX = 12
	FINISH_TEST = 13

def main ():
	args = parse_args()

	test_id = mistress_server_start_test(args.test_name, args.stat_host)
	def send_stat (data, step):
		data = zlib.compress(json.dumps({'node': 1, 'step': step, 'data': data}))
		requests.post('http://%s/add_stats/%s' % (args.stat_host, test_id), data = data)
	
	cmd = [args.executable_path, args.test_name]
	rtmp_load_proc = subprocess.Popen(cmd, shell = False, stdout = subprocess.PIPE, bufsize = -1)
	# cmd = "strace -f -o strace.log %s %s" % (args.executable_path, args.test_name)
	# rtmp_load_proc = subprocess.Popen(cmd, shell = True, stdout = subprocess.PIPE, bufsize = -1)

	try:
		proc_terminated = False

		second = datetime.timedelta(seconds = 1)
		buf_buffered_frame_num = []
		# buf_first_frame_latency = []
		concur_threads = 0
		buf_thread_started_num = 0

		stat_buf = []
		step = 1

		print >> sys.stderr, "columns:\nbuf_thread_started_num, concur_threads, med_buffered_frame_num, med_first_frame_latency:"
		start_time = datetime.datetime.utcnow()
		while True:
			line = rtmp_load_proc.stdout.readline()
			if not line:
				break 

			rtype, t, parts = parse_line(line)

			if rtype == '@buf_frame_num':
				n = int(parts[3])
				buf_buffered_frame_num.append(n)

				if n < 0:
					stat_buf.append({'type': stypes.RESPONSE_ERROR, 'value': "not enough frames"})
			elif rtype == '@starting_thread':
				buf_thread_started_num += 1
				concur_threads += 1
			elif rtype == '@stopping_thread':
				stat_buf.append({'type': stypes.RESPONSE_ERROR, 'value': "frame receiving error: %s" % " ".join(parts[3:])})
				concur_threads -= 1	
			# elif rtype == '@diff':
			# 	#TODO ?
			# 	pass
			elif rtype == '@first_frame':
				msec = int(parts[3]) * 1000 + int(int(parts[4]) / 1000000) #TODO is it correct?
				# buf_first_frame_latency.append(msec)
				
				stat_buf.append({'type': stypes.RESPONSE_TIME, 'value': ('first frame, ms', msec)})
				stat_buf.append({'type': stypes.RESPONSE_STATUS, 'value': ('first frame, rate', 200)})
			elif rtype == '@error':
				stat_buf.append({'type': stypes.RESPONSE_ERROR, 'value': " ".join(parts[3:])})
			else:
				raise RuntimeError("Invalid event type: %s" % rtype)

			now = datetime.datetime.utcnow()
			d = now - start_time
			if d >= second: #TODO reminder
				# print "reminder", d - second
				start_time = now

				# process metrics
				med_buffered_frame_num = get_median(buf_buffered_frame_num) if buf_buffered_frame_num else 0
				# med_first_frame_latency = get_median(buf_first_frame_latency) if buf_first_frame_latency else 0
				# print buf_thread_started_num, concur_threads, med_buffered_frame_num, med_first_frame_latency #.......
				print buf_thread_started_num, concur_threads, med_buffered_frame_num

				stat_buf.extend([
					{'type': stypes.START_SESSION, 'value': buf_thread_started_num},
					{'type': stypes.CONCUR_CONNS_NUM_MIN, 'value': concur_threads},
					{'type': stypes.CONCUR_CONNS_NUM_MAX, 'value': concur_threads},
					{'type': stypes.CONCUR_USERS_NUM_MIN, 'value': concur_threads},
					{'type': stypes.CONCUR_USERS_NUM_MAX, 'value': concur_threads},
					{'type': stypes.REQUEST_SENT, 'value': med_buffered_frame_num},
				])
				send_stat(stat_buf, step)
				step += 1

				#reset buffers
				buf_thread_started_num = 0
				buf_buffered_frame_num = []
				# buf_first_frame_latency = []
				stat_buf = []

		proc_terminated = True

		rtmp_load_proc.poll()
		if rtmp_load_proc.returncode != 0:
			print >> sys.stderr, "rtmp_load returned non-zero exit code %s" % (rtmp_load_proc.returncode if rtmp_load_proc.returncode is not None else "None (possibly segfault)")
			sys.exit(1)
	finally:
		if not proc_terminated:
			try:
				rtmp_load_proc.send_signal(signal.SIGINT)
			except Exception as e:
				print >> sys.stderr, "failed to send sigint: %s" % e

		if stat_buf:
			print >> sys.stderr, "last data in stat buf: %s", stat_buf
			print >> sys.stderr, "possibly latest error: %s", stat_buf[-1]
			send_stat(stat_buf, step)

		send_stat([{'type': stypes.FINISH_TEST, 'value': 1}], step)


if __name__ == '__main__':
	main()