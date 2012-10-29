import re
import sys


RE_DEF = re.compile(r'#define ([\w_]+) (0x.+|\d+(?!U?L)(?!\d+))')
RE_UNDEF = re.compile(r'#undef ([\w_]+)')


lines = sys.stdin.read().splitlines(True)


defs = {}
res = []

for l in lines:
	if l.startswith('#undef'):
		m = RE_UNDEF.match(l).group(1)
		if m in defs:
			#sys.stderr.write('**undef ' + m + ('(%s)' % defs[m]) + '\n')
			del defs[m]
	elif l.startswith('#define'):
		m = RE_DEF.match(l)
		if m:
			k, v = m.groups()
			defs[k] = v
	else:
		res.append(l)

sys.stdout.write(''.join(res))
sys.stdout.write('enum {' + ',\n'.join('%s = %s' % p for p in defs.items()) + '};')

