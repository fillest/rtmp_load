#!/usr/bin/env python
import subprocess
import sys
import signal
import datetime
import argparse


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
	parser.add_argument('-b', '--binary-path', default = 'build/dev/rtmp_load', help = "rtmp_load binary path")
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

def main ():
	args = parse_args()

	cmd = [args.binary_path, args.test_name]
	process = subprocess.Popen(cmd, shell = False, stdout = subprocess.PIPE, bufsize = -1)
	
	try:
		now = datetime.datetime.utcnow()
		delta_second = datetime.timedelta(seconds = 1)
		buf_buffered_frame_num = []
		buf_first_frame_latency = []
		concur_threads = 0
		buf_new_threads_num = 0
	
		print >> sys.stderr, "(columns) buf_new_threads_num, concur_threads, med_buffered_frame_num, med_first_frame_latency:"
		while True:
			line = process.stdout.readline()
			if line:
				rtype, t, parts = parse_line(line)
				
				if rtype == '@buffered_frame_num':
					buf_buffered_frame_num.append(int(parts[3]))
				elif rtype == '@starting_thread':
					buf_new_threads_num =+ 1
					concur_threads += 1
				elif rtype == '@diff':
					#TODO ?
					pass
				elif rtype == '@first_frame':
					msec = int(parts[3]) * 1000 + int(int(parts[4]) / 1000000) #TODO is correct?
					buf_first_frame_latency.append(msec)
				elif rtype == '@error':
					print '@error', parts[3]
				else:
					raise ValueError(rtype)

				if t - now >= delta_second:
					now = datetime.datetime.utcnow()
					# compute stat and flush buffers
					med_buffered_frame_num = get_median(buf_buffered_frame_num) if buf_buffered_frame_num else 0
					buf_buffered_frame_num = []
					med_first_frame_latency = get_median(buf_first_frame_latency) if buf_first_frame_latency else 0
					buf_first_frame_latency = []
					print buf_new_threads_num, concur_threads, med_buffered_frame_num, med_first_frame_latency #.......
					buf_new_threads_num = 0
			else:
				break

		if process.returncode:
			print >> sys.stderr, "@returncode %s" % process.returncode
			sys.exit(1)
	except:
		process.send_signal(signal.SIGINT)
		raise


if __name__ == '__main__':
	main()