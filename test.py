#!/usr/bin/env python
import sys,os,time
from termios import *

cmd_open="zb\x01"
cmd_close="zb\x02"
cmd_set_channel="zb\x04"
cmd_ed="zb\x05"
cmd_cca="zb\x06"
cmd_set_state="zb\x07"
data_xmit_block="zb\x09"
resp_recv_block="zb\x0b"

ispeed = 4 
ospeed = 5

IDLE_MODE = 0
RX_MODE = 2
TX_MODE = 3

class DQ:
	def __init__(self, port):
		try:
			print "Openning " + port
			self.file = os.open(port, os.O_RDWR|os.O_NOCTTY)
		except IOError:
			print "IOError in open"
			sys.exit(2)

		try:
			self.oldattrs = tcgetattr(self.file)
			attrs = [IGNPAR, self.oldattrs[1], 0, self.oldattrs[3], B115200, B115200, self.oldattrs[6]]
			attrs[2] = B115200 | CS8 | CLOCAL | CREAD
			attrs[6][VTIME] = 0
			attrs[6][VMIN] = 1
			tcflush(self.file, TCIFLUSH)
			tcsetattr(self.file, TCSANOW, attrs)
		except IOError:
			print "IOError in termios"
			sys.exit(2)
	def parse_status(self):
		if self.status == 0:
			self.strstatus = "SUCCESS"
		elif self.status == 1:
			self.strstatus = "RX_ON"
		elif self.status == 2:
			self.strstatus = "TX_ON"
		elif self.status == 3:
			self.strstatus = "TRX_OFF"
		elif self.status == 4:
			self.strstatus = "IDLE"
		elif self.status == 5:
			self.strstatus = "BUSY"
		elif self.status == 6:
			self.strstatus = "BUSY_RX"
		elif self.status == 7:
			self.strstatus = "BUSY_TX"
		elif self.status == 8:
			self.strstatus = "ERR"


	def write(self, string):
		print "Writing "+string
		os.write(self.file, string)

	def read(self, num):
		v =  os.read(self.file, num)
		print "Reading %s %02x" %(v, ord(v))
		return v

	def response(self):
		state = 1
		while 1:
			val = self.read(1)
			print val
			if state == 1:
				if val == 'z':
					state = 2
				elif val == '\000':
					state = 1
				else:
					print "Bad character: %s %02x" % (val, ord(val))
			elif state == 2:
				if val == 'b':
					state = 3
				elif val == '\000':
					state = 1
				else:
					print "Bad character: %s %02x" % (val, ord(val))
			elif state == 3:
				id = ord(val)
				if id == 0x85:
					self.status = ord(self.read(1))
					self.data = self.read(1)
				elif id == 0x8b:
					self.status = ord(self.read(1))
					len = self.read(1)
					self.data = self.read(len)
				else:
					self.status = ord(self.read(1))
					self.data = None
				self.parse_status()
				return id
	def open(self):
		self.write(cmd_open)
		v = self.response()
		if v != 0x81:
			print "Returned invalid id value " +v
			raise IOError
		else:
			return self.status
	def close(self):
		self.write(cmd_close)
		v = self.response()
		if v != 0x82:
			print "Returned invalid id value " +v
			raise IOError
		else:
			return self.status
	def ed(self):
		self.write(cmd_ed)
		v = self.response()
		if v != 0x85:
			print "Returned invalid id value " +v
			raise IOError
		else:
			return self.status
	def set_channel(self, channel):
		self.write(cmd_set_channel+chr(channel))
		v = self.response()
		if v != 0x84:
			print "Returned invalid id value " +v
			raise IOError
		else:
			return self.status
	def set_state(self, mode):
		self.write(cmd_set_state+chr(mode))
		v = self.response()
		if v != 0x87:
			print "Returned invalid id value " +v
			raise IOError
		else:
			return self.status
	def send_block(self, data):
		self.write(data_xmit_block+chr(len(data))+data)
		v = self.response()
		if v != 0x89:
			print "Returned invalid id value " +v
			raise IOError
		else:
			return self.status



if len(sys.argv) < 2:
	print "Bad arguments"
	sys.exit(2)

cn = DQ(sys.argv[1])
print 'Result of close ' + hex(cn.close())
print 'Result of open ' + hex(cn.open())
#res = cn.set_state(IDLE_MODE)
#if res == 5 or res == 8:
#	print "Unable to set IDLE mode :("
#	cn.close()
#	sys.exit(2)
#
#res = cn.set_state(RX_MODE)
#if res == 5 or res == 8:
#	print "Unable to set RX mode :("
#	cn.close()
#	sys.exit(2)
# print 'Result of set_state ' + hex(cn.set_state(RX_MODE))
try:
	while 1:
		print 'Result of set_channel' +hex(cn.set_channel(4))
		print 'Result of set_state' +hex(cn.set_state(TX_MODE))
		print 'Result of send_block' +hex(cn.send_block("zzz"))
except KeyboardInterrupt:
		cn.close()
		sys.exit(2)


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
	print 'Result of ed ' + hex(cn.ed()) + ' ' + hex(ord(cn.data))
print 'Result of close ' + hex(cn.close())
sys.exit(2)

#state = 0
#try:
#	f.write(cmd_open)
#except IOError:
#	print "Error on write"
#	sys.exit(2)
#
#resp = get_response(f)
#print "got response %d" % (resp);
#sys.exit(2)
#
#try:
#	state = 0
#	while 1:
#		if state == 0:
#			f.write(cmd_open)
#			state = 1
#		val = f.read(1)
#except KeyboardInterrupt:
#	f.close()
#

