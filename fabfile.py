from fabric.api import run, env, parallel, settings, local, hosts, task
from fabric.tasks import execute
from fabric.operations import put
from fabric.context_managers import cd, prefix


env.linewise = True  #for parallel execution

path = '~/rtmp_load'  #TODO hardcode


@task
@hosts('')
def pack ():
	local('tar --exclude="build" --exclude="*.log" --exclude="venv" --exclude="*.pyc" --exclude="*.a" --exclude="*.o" --exclude=".bold_state" --exclude-vcs -czf rtmp_load.tar.gz *')

@task
@parallel
def upload ():
	run('sudo apt-get install -y librtmp0 librtmp-dev yasm pkg-config python-virtualenv')

	put('rtmp_load.tar.gz', '/tmp')
	try:
		run('rm -rf {path}/src && mkdir -p {path} && tar -xf /tmp/rtmp_load.tar.gz -C {path}'.format(path = path))  
	finally:
		run('rm /tmp/rtmp_load.tar.gz')

	with cd(path):
		run('test -f venv/bin/activate'
			' || (virtualenv --no-site-packages venv'
			' && source venv/bin/activate'
			#' && pip install --upgrade pip'
			#' && pip install git+https://github.com/fillest/bold.git'
			' && pip install -r requirements.txt)')
		with prefix('source venv/bin/activate'):  #TODO (?)
			run('bold')

@task
@hosts('')
def clean ():
	local('rm -f rtmp_load.tar.gz')

@task
@hosts('')
def deploy ():
	try:
		execute(pack)
		execute(upload)
	finally:
		execute(clean)

@task
def start ():
	with cd(path):
		with prefix('source venv/bin/activate'):
			run('bold && ./run.py test.lua')