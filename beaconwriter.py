import sys
import subprocess
from subprocess import Popen, PIPE, STDOUT
import re

pat = re.compile('\\\\n')

while True:
    line = sys.stdin.readline()
    p = Popen(['/usr/bin/write','baitisj'], stdout=PIPE, stdin=PIPE, stderr=PIPE)
    p.communicate(input=pat.sub('\n', line))
