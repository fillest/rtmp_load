import bold
from bold import build_path
import os
import sys
import multiprocessing


class Libav (bold.builders.Builder):
	src_path = 'src/libav/'
	target = src_path + 'libavformat/libavformat.a'
	required_by = lambda: Rtmp_load.target
	sources = src_path + 'VERSION'

	def build (self, _changed_targets, _src_paths):
		with bold.util.change_cwd(self.src_path):
			#--disable-debug --enable-gpl --enable-nonfree
			self.shell_run("./configure --enable-librtmp --disable-doc --disable-ffmpeg --disable-avconv"
				" --disable-avplay --disable-avprobe --disable-avserver --disable-swscale --disable-avdevice"
				" --disable-avfilter --enable-runtime-cpudetect --disable-encoders"
				" && make --jobs %i" % multiprocessing.cpu_count())
		self._update_target(self.target)

class Luajit (bold.builders.Builder):
	src_path = 'src/luajit'
	target = src_path + '/src/libluajit.a'
	required_by = lambda: Rtmp_load.target
	sources = lambda self: list(bold.util.get_file_paths_recursive(self.src_path, [self.src_path + '/doc/*']))

	def build (self, _changed_targets, _src_paths):
		with bold.util.change_cwd(self.src_path):
			self.shell_run('''make amalg CCDEBUG=" -g" BUILDMODE=" static"''')
		self._update_target(self.target)

class Rtmp_load (bold.builders.CProgram):
	target = build_path + 'rtmp_load'
	sources = 'src/*.c'
	compile_flags = '-O3 -g -std=gnu99 -Wall -pthread -DUSE_SYSEXITS'  # USE_SYSEXITS is for gopt
	includes = [
		# '/home/f/proj/libav-0.8.3/i/include/',
		Libav.src_path,
		# '/usr/include/lua5.1/',
		Luajit.src_path + '/src',
	]
	link_flags = '-Wl,--export-dynamic' #-Wl,-rpath=/home/f/proj/libav-0.8.3/i/lib/
	lib_paths = [
		# '/home/f/proj/libav-0.8.3/i/lib/',
		Libav.src_path + 'libavformat',
		Libav.src_path + 'avcodec',
		Libav.src_path + 'avutil',
		Luajit.src_path + '/src',
	]
	libs = [
		'rt',
		'avformat',
		'avcodec',
		'avutil',
		'rtmp',
		'luajit',
	]

class FFIHeader (bold.builders.Builder):
	sources = 'ffi.py', 'src/run.h'
	target = build_path + 'luajit_ffi.h'
	required_by = Rtmp_load.target

	def build (self, changed_targets, src_paths):
		headers = "\n".join([
			"#include <time.h>",
			"#include <pthread.h>",
			'#include \\"src/run.h\\"',
		])
		self.shell_run("bash -o pipefail -c \"echo '" + headers + "' | gcc -E -dD - | grep -v '^# ' | python ffi.py > " + self.target + '"')
		self._update_target(self.target)