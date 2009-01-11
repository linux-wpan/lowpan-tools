#!/usr/bin/env python
import sys,os,time
from termios import *
from test_DQ import *

if len(sys.argv) < 2:
	print "Bad arguments"
	sys.exit(2)

cn = DQ(sys.argv[1])
print 'Result of open ' + hex(cn.open())
for i in range(1, 12):
	print 'Result of set_channel ' + hex(cn.set_channel(i))
	time.sleep(1)
	m = 0
	res = 5
	while res != 0 or m > 60:
		res = cn.set_state(RX_MODE)
		print "Got res %d" %(res)
		m = m + 1
		time.sleep(1)
	if res == 5 or res == 8:
		print "Unable to set RX mode :("
		cn.close()
		sys.exit(2)
	print 'Result of ed for ' + str(i) + ' is ' + hex(cn.ed()) + ' ' + hex(ord(cn.data))
#	print 'Result of cca ' + str(i) + ' is ' + hex(cn.cca()) + ' ' + cn.strstatus
print 'Result of close ' + hex(cn.close())
sys.exit(2)

