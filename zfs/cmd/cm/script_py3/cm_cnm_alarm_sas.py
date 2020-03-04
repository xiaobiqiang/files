#!/bin/python
#-*- coding: UTF-8 -*-
import sys
import os
import subprocess

global updateFile

def execCmd(cmd):
    fd = subprocess.Popen(cmd,shell=True,stdout=subprocess.PIPE)
    result = fd.stdout.read()
    return result.decode('utf-8')

def init():
    if os.path.isfile(updateFile):
        return
    
    fd = open(updateFile,'w')
    #ret,sesList = commands.getstatusoutput('ls /dev/es/|egrep -v ses0')
    sesList = execCmd('ls /dev/es/|egrep -v ses0')
    if len(sesList.strip()) == 0:
        os.system('touch '+updateFile)
        return
        
    sesList = sesList.split('\n')
    for ses in sesList:
        if len(ses.strip()) == 0:
            continue
        ses='/dev/es/'+ses
        id = execCmd("sg_ses -p1 "+ses+" 2>/dev/null|grep hex|awk '{print $5}'")
        fd.write(id)
        fd.flush()
    fd.close()    
    return

def alarm_thread():
    diskIdList = []
    hostname = execCmd('hostname')
    alarmId = 10000016
    fd = open(updateFile)
    for line in fd:
        diskIdList.append(line.strip())
    fd.close()
      
    sesList = execCmd('ls /dev/es/|egrep -v ses0')
    sesList = sesList.split('\n')
    for ses in sesList:
        if len(ses) == 0:
            continue
        ses='/dev/es/'+ses
        id = execCmd("sg_ses -p1 "+ses+" 2>/dev/null|grep hex|awk '{print $5}'")
        id = id.strip()
        if len(id) == 0:
            continue
        if id in diskIdList:
            diskIdList.remove(id)
        else:
            #恢复告警
            sn = execCmd("sqlite3 /var/cm/data/cm_topo.db \"select sn from device_t where device='"+id+"'\"")
            param = hostname.strip()+","+sn.strip()
            os.system("ceres_cmd alarm recovery "+str(alarmId)+" '"+param+"'")
                 
    for id in diskIdList:
        #上报告警
        sn = execCmd("sqlite3 /var/cm/data/cm_topo.db \"select sn from device_t where device='"+id+"'\"")
        param = hostname.strip()+","+sn.strip()
        os.system("ceres_cmd alarm report "+str(alarmId)+" '"+param+"'")
        
    if os.path.isfile(updateFile):
        os.system('rm '+updateFile)
    init()
    return
 

    
if __name__ == '__main__':
    updateFile = '/var/cm/data/cm_alarm_sas'
    if len(sys.argv) != 2:
        os.exit(1)
    if sys.argv[1] == 'init':
        init()
    elif sys.argv[1] == 'thread':
        alarm_thread()
    else:
        pass
