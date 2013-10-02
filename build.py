import bold
from bold import build_path, util
import multiprocessing
import shutil


class Libav (bold.builders.Builder):
	required_by = lambda: Rtmp_load.target

	src_path = 'src/libav'
	sources = lambda self: list(util.get_file_paths_recursive(self.src_path))
	src_copy_path = build_path + 'libav_src'
	target = src_copy_path + '/libavformat/libavformat.a'

	def build (self, _changed_targets, _src_paths):
		shutil.rmtree(self.resolve(self.src_copy_path), ignore_errors = True)
		shutil.copytree(self.src_path, self.resolve(self.src_copy_path))

		with util.change_cwd(self.resolve(self.src_copy_path)):
			#--disable-debug --enable-gpl --enable-nonfree
			self.shell_run("./configure --enable-librtmp --disable-doc --disable-ffmpeg --disable-avconv"
				" --disable-avplay --disable-avprobe --disable-avserver --disable-swscale --disable-avdevice"
				" --disable-avfilter --enable-runtime-cpudetect --disable-encoders"
				" && make --jobs %i" % (multiprocessing.cpu_count() * 2))
		self._update_target(self.target)

class Luajit (bold.builders.Builder):
	required_by = lambda: Rtmp_load.target

	src_path = 'src/luajit'
	src_copy_path = build_path + 'luajit_src'
	target = src_copy_path + '/src/libluajit.a'
	sources = lambda self: list(util.get_file_paths_recursive(self.src_path, [self.src_path + '/doc/*']))

	def build (self, _changed_targets, _src_paths):
		shutil.rmtree(self.resolve(self.src_copy_path), ignore_errors = True)
		shutil.copytree(self.src_path, self.resolve(self.src_copy_path))

		with util.change_cwd(self.resolve(self.src_copy_path)):
			self.shell_run('''make amalg CCDEBUG=" -g" BUILDMODE=" static"''')
		self._update_target(self.target)

class Rtmp_load (bold.builders.CProgram):
	target = build_path + 'rtmp_load'
	sources = 'src/*.c'
	deps_include_missing_header = True
	gopt_flags = '-DUSE_SYSEXITS'
	compile_flags = '-O3 -g -std=gnu99 -Wall -pthread ' + gopt_flags
	includes = [
		Libav.src_copy_path,
		# '/usr/include/lua5.1/',
		Luajit.src_path + '/src',
	]
	link_flags = '-Wl,--export-dynamic' #-Wl,-rpath=/home/f/proj/libav-0.8.3/i/lib/
	lib_paths = [
		Libav.src_copy_path + '/libavformat',
		Libav.src_copy_path + '/libavcodec',
		Libav.src_copy_path + '/libavutil',
		Luajit.src_copy_path + '/src',
	]
	libs = [
		'rt',
		'dl',
		'avformat',
		'avcodec',
		'avutil',
		'rtmp',
		'luajit',
		'm',
	]

class FFIHeader (bold.builders.Builder):
	required_by = Rtmp_load.target

	sources = 'ffi.py', 'src/run.h'
	target = build_path + 'luajit_ffi.h'

	def build (self, changed_targets, src_paths):
		headers = "\n".join([
			"#include <time.h>",
			"#include <pthread.h>",
			'#include \\"src/run.h\\"',
		])
		self.shell_run("bash -o pipefail -c \"echo '" + headers + "' | gcc -E -dD - | grep -v '^# ' | python ffi.py > " + self.target + '"')
		self._update_target(self.target)