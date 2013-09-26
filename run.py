#!/usr/bin/env python
import subprocess
import sys
import signal
import datetime
import argparse


def get_median (l):
	sv = sorted(l)
	if len(sv) % 2 == 1:
		return sv[(len(sv) + 1) / 2 - 1]
	else:
		lower = sv[len(sv) / 2 - 1]
		upper = sv[len(sv) / 2]

		return (float(lower + upper)) / 2

def main ():
	parser = argparse.ArgumentParser()
	parser.add_argument('test', help = "test file path")
	parser.add_argument('-b', '--binary-path', default = 'build/dev/rtmp_load', help = "rtmp_load binary path")
	args = parser.parse_args()


	cmd = [args.binary_path, args.test]
	process = subprocess.Popen(cmd, shell = False, stdout = subprocess.PIPE, bufsize = -1)

	now = datetime.datetime.utcnow()
	delta_second = datetime.timedelta(seconds = 1)
	buf_buffered_frame_num = []
	buf_first_frame_latency = []
	concur_threads = 0
	buf_new_threads_num = 0
	try:
		print >> sys.stderr, "buf_new_threads_num, concur_threads, med_buffered_frame_num, med_first_frame_latency:"
		while True:
			line = process.stdout.readline()
			if line:
				parts = line.strip().split()
				try:
					t = datetime.datetime.strptime(parts[0] + ' ' + parts[1], '%Y-%m-%d %H:%M:%S.%f')
				except:
					print >> sys.stderr, "Invalid line:", line 
					raise

				rtype = parts[2]
				if rtype == '@buffered_frame_num':
					buf_buffered_frame_num.append(int(parts[3]))
				elif rtype == '@starting_thread':
					buf_new_threads_num =+ 1
					concur_threads += 1
				elif rtype == '@diff':
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
	except KeyboardInterrupt:
		# print >> sys.stderr, "interrupted"
		pass
	except:
		process.send_signal(signal.SIGINT)
		raise


if __name__ == '__main__':
	main()