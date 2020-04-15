#!/usr/bin/python

import os, sys, time, getopt
import random
import threading

dbgmsgs_path = "/proc/spl/kstat/zfs/dbgmsg"
logoutpath = "/tmp/result.log"

filter = ["txg_sync_thread", "txg_quiesce_thread"]

m_buffer = []
m_buf_maxlen = 100
dedup = 0

def m_buf_insert(m):
	global m_buffer
	global dedup
	dup = False
	if m in m_buffer:
		dup = True
		dedup += 1
		# print "dup message: " + m
	m_buffer.append(m)
	if len(m_buffer) > m_buf_maxlen:
		m_buffer.pop(0)
	return dup

def m_buf_clear():
	global m_buffer
	global dedup
	m_buffer = []
	# print "dedup %d" % (dedup)
	dedup = 0

def exec_comm(comm):
	f = os.popen(comm)
	text = f.read()
	f.close()
	return text

def trace_dbgmsgs(path):
	ts = 0
	while True:
		f = open(dbgmsgs_path, 'r')
		ofile = open(path, 'a')
		line = 0
		t = ts
		fl = []
		repeat = 0
		while True:
			s = f.readline()
			if len(s) == 0:
				break;
			line += 1
			if line < 3:
				continue
			a = s.split()
			t = int(a[0])
			if ts >= t:
				continue
			if a[1] in filter:
				continue
			if len(fl) > 0 and a[1] == fl[0] and a[2] == fl[1]:
				repeat += 1
				continue
			if repeat > 0:
				buf = "last message repeat %d times\n" % (repeat)
				# ofile.write(buf)
				# print buf,
				repeat = 0
			fl = [a[1], a[2]]
			msg = s[s.find(a[1]):]
			if m_buf_insert(msg) is False:
				buf = "%s %s" % (time.ctime(t), msg)
				ofile.write(buf)
				# print buf,
		m_buf_clear()
		f.close()
		ofile.close()
		ts = t
		time.sleep(10)

def open_dbgmsg_trace():
	result = exec_comm("cat /sys/module/zfs/parameters/zfs_dbgmsg_enable")
	if result[0] == '0':
		exec_comm("echo 1 > /sys/module/zfs/parameters/zfs_dbgmsg_enable")

def main():
	try:
		opts, args = getopt.getopt(sys.argv[1:], "p:")
	except getopt.GetoptError:
		print sys.argv[0] + ' -p <path>'
		sys.exit(2)
	path = logoutpath
	for opt, arg in opts:
		if opt == '-p':
			path = arg
	open_dbgmsg_trace()
	trace_dbgmsgs(path)

if __name__ == "__main__":
	main()