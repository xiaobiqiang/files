#!/bin/python
#-*- coding: UTF-8 -*-
import sys
import commands
import select
import socket
import os
import threading
import time


#node state
#0 初始状态
#1 正在传输rd包中
#2 传输完成等待升级
#3 升级包解压成功
#4 备份关键数据成功
#5 更新rd包成功
#6 重启状态
#7 节点上线，升级完成
#9 错误状态

class Node:
    def __init__(self,name,ip,state):
        self.name = name 
        self.ip = ip
        self.state = state
        self.process = 0

class Server:
    def __init__(self,port,host):
        self.port = port
        self.host = host
        self.loop = True
        self.updateThread = True
        self.startFlag = 0 #1是离线升级 2是在线
        self.isUpgrade = True #true是升级 false是补丁
        self.serfd = ''
        self.nodeList = []
        self.inputs = []
        self.myself = ''
        self.rdDir = ''
        self.updateFile = '/var/cm/data/upgrade.txt'
        self.scriptDir = '/var/cm/script/cm_upgrade.py'
        self.shellDir = '/var/cm/script/cm_cnm_upgrade.sh'
        self.upgradesh = '/var/nasadmin/ugtest.sh'
        self.rdFileDir = '/tmp/upgrade/'
        self.rdbFile = '/var/cm/data/upgrade_rdb.txt'
        self.loopTime = 3600 #升级服务存活时间,3600秒
        
        self.cmdMap = {
            'exit' : self.exit,
            'starton' : self.starton,
            'startoff' : self.startoff,
            'get' : self.get,
            'update' : self.update
        }
        
        self.stateMap = {
            0 : 'init',
            1 : 'transfer',
            2 : 'wait',
            3 : 'unpack',
            4 : 'bak',
            5 : 'end',
            6 : 'reboot',
            7 : 'poweron',
            9 : 'error'
        }
        
    def exit(self):
        self.loop = False
        self.updateThread = False
        
        if os.path.isfile(self.updateFile):
            os.system("rm "+self.updateFile)
            
        if os.path.isfile(self.rdbFile):
            os.system("rm "+self.rdbFile)
        
        return

    def starton_to_next(self):
        print "starton_to_next"
        endFlag = True
        for node in self.nodeList:
            print node.name+" "+str(node.state)
            if node.state == 2 and node.name != self.myself:
                os.system("cm_multi_exec "+node.name+" '"+self.scriptDir+" starton'")#向下一个节点发送升级命令
                endFlag = False
                break
        os.system('rm -rf '+self.rdFileDir)
        if endFlag:
            #在线升级的最后一个节点，发送命令使所有节点升级服务退出
            os.system("cm_multi_exec "+" '"+self.scriptDir+" exit'")
            print "exit"
     
    def starton(self):
        if not os.path.isfile(self.rdDir):
            print "not rdDir"
            return
        self.startFlag = 2
        mynode = self.node_find(self.myself)
        if mynode.state == 0:
            os.system(self.scriptDir+" update"+str(1))
        print 'starton'
        
        return
            
    def startoff(self):
        if not os.path.isfile(self.rdDir):
            print "not rdDir"
            return
        self.startFlag = 1
        os.system(self.scriptDir+" update"+str(1))
                   
        return 

    def get(self):
        result = ''
        for node in self.nodeList:
            result+=node.name+" "+str(node.process)+" "+str(node.state)+"\n"
        return result
    
    def update(self,ip,state):
        hostname = ''
        if ip == '127.0.0.1':
            hostname = self.myself
        else:
            ret,hostname = commands.getstatusoutput("grep "+ip+" /etc/clumgt.config|awk '{printf $1}'")
        
        self.node_update_state(hostname,state)
    
        return
        
    def rdb_get(self):
        if not os.path.isfile(self.rdbFile):
            return 
        fd = open(self.rdbFile)
        for line in fd:
            name,ip,process,state = line.split(' ')
            node = Node(name,ip,int(state.strip()))
            self.nodeList.append(node)
        return
        
    def rdb_set(self):
        fd = open(self.rdbFile,'w')
        for node in self.nodeList:
            line = node.name+" "+node.ip+" "+str(node.process)+" "+str(node.state)+"\n"
            fd.write(line)
            fd.flush()
        return
        
    def upgrade_process(self):
        os.system(self.shellDir+" process "+self.rdDir+" &")
        os.system(self.scriptDir+" update"+str(3))
        return 
        
    def node_find(self,name):
        for node in self.nodeList:
            if node.name == name:
                return node
        return None
            
        
    def node_update_thread(self):
        count = 0
        while self.updateThread:
            if count >= self.loopTime:
                self.exit()
            count += 1
            mynode = self.node_find(self.myself)
            if self.startFlag == 1 and mynode.state == 1:
                os.system(self.scriptDir+" update"+str(2))
                for node in self.nodeList:
                    if node.name == self.myself:
                        continue
                    os.system("cm_multi_exec "+node.name+" '"+self.scriptDir+" update"+str(1)+"'")
                    os.system("cm_multi_send "+node.name+" "+self.rdDir+" "+self.rdFileDir)#传输rd包
                    os.system("cm_multi_exec "+node.name+" '"+self.scriptDir+" update"+str(2)+"'")#更改该节点状态
                for node in self.nodeList:
                    os.system("cm_multi_exec "+node.name+" '"+self.shellDir+" process "+self.rdDir+"' &")#集群执行升级脚本
            
            if self.startFlag == 2 :
                if mynode.state == 1:
                    os.system(self.scriptDir+" update"+str(2))
                    for node in self.nodeList:
                        if node.name == self.myself:
                            continue
                        os.system("cm_multi_exec "+node.name+" '"+self.scriptDir+" update"+str(1)+"'")
                        os.system("cm_multi_send "+node.name+" "+self.rdDir+" "+self.rdFileDir)#传输rd包
                        os.system("cm_multi_exec "+node.name+" '"+self.scriptDir+" update"+str(2)+"'")#更改该节点状态
                elif mynode.state == 2:
                    self.upgrade_process()
                else:
                    pass
                
            if not os.path.isfile(self.updateFile):
                time.sleep(1)
                continue
                
            ret,flag = commands.getstatusoutput("cat "+self.updateFile)
            flag = flag.strip()
            if flag == self.stateMap[mynode.state]:
                time.sleep(1)
                continue
            
            if flag == 'unpack':
                os.system(self.scriptDir+" update"+str(3))
            elif flag == 'bak':
                os.system(self.scriptDir+" update"+str(4))
            elif flag == 'end':
                os.system(self.scriptDir+" update"+str(5))
                if self.startFlag == 1 or self.startFlag == 0:#离线升级
                    os.system('rm '+self.updateFile)
                    os.system('rm -rf '+self.rdFileDir)
                    self.exit()
                    os.system(self.scriptDir+" update"+str(6))#重启节点
                    os.system('reboot')
                
                if self.startFlag == 2:#在线升级
                    os.system('rm '+self.updateFile)
                    self.rdb_set()
                    os.system(self.scriptDir+" update"+str(6))#重启节点
                    os.system('reboot')
            elif flag == 'error':
                    os.system(self.scriptDir+" update"+str(9))
                    #发生错误，回退版本
                    os.system(self.shellDir+" back")
            else:
                pass
            time.sleep(1)
        return 
    
    def node_update_process(self,node,state):
        if state == 0:
            node.process = 0
        elif state == 1:
            node.process = 10
        elif state == 2:
            node.process = 30
        elif state == 3:
            node.process = 50
        elif state == 4:
            node.process = 80
        elif state == 5:
            node.process = 100
        else:
            pass
            
    
    def node_update_state(self,name,state):
        if name == 'all':
            for node in self.nodeList:
                node.state = state
                self.node_update_process(node,state)
            return
    
        for node in self.nodeList:
            if node.name == name:
                node.state = state
                self.node_update_process(node,state)
                break
        return 
        
    def node_init(self):
        if os.path.isfile(self.rdbFile):
            self.rdb_get()
            ret,self.rdDir = commands.getstatusoutput("ls "+self.rdFileDir+"|egrep -v unpack|awk '{printf $1}'")
            self.rdDir = self.rdFileDir+self.rdDir
            os.system(self.scriptDir+" update"+str(7))#升级完成，节点上线
            self.starton_to_next()
            os.system('rm '+self.rdbFile)
            return
            
        fd = open('/etc/clumgt.config')
        for line in fd:
            hostname,ip = line.split(' ')
            node = Node(hostname,ip.strip(),0)
            self.nodeList.append(node)
        
    def server_process(self):
        self.serfd = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        self.serfd.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
        localAddr = ('',self.port)
        self.serfd.bind(localAddr)
        self.serfd.listen(100)
        self.inputs = [self.serfd]
        
        ret,self.myself = commands.getstatusoutput("hostname|awk '{printf $1}'")
        self.node_init()
        handle_t = threading.Thread(target=self.node_update_thread,args=())
        handle_t.start()
        
        while self.loop:
            try:
                readable,writeable,exceptionable = select.select(self.inputs,[],[])
            except:
                self.inputs = [self.serfd]
                continue
            else:        
                for sock in readable:
                    if sock == self.serfd:
                        client,addr = self.serfd.accept()
                        clientip = addr[0]
                        data = client.recv(2048)
                        cmd = ''
                        if data[0] == '/':
                            self.rdDir = data
                            if not os.path.isdir(self.rdFileDir):
                                os.system('mkdir -p '+self.rdFileDir)
                            fileName = os.path.basename(self.rdDir)
                            self.rdDir = self.rdFileDir+fileName
                            print self.rdDir
                            continue
                        else:
                            cmd = data
                            
                        if cmd == 'get':
                            client.sendall(self.cmdMap[cmd]())
                        elif cmd[:-1] == 'update':
                            self.cmdMap[cmd[:-1]](clientip,int(cmd[-1]))
                        else:
                            self.cmdMap[cmd]()

        

def upgrade_server():
    
    pid = os.fork()
    if pid:
        sys.exit(0)
    os.umask(0)
    os.setsid()
    cpid = os.fork()
    if cpid:
        sys.exit(0)
    os.chdir('/')
    sys.stdout.flush()
    sys.stderr.flush()
        
    readNull = open('/dev/null')
    writeNull = open('/dev/null','w')
    os.dup2(readNull.fileno(),sys.stdin.fileno())
    os.dup2(writeNull.fileno(),sys.stdout.fileno())
    os.dup2(writeNull.fileno(),sys.stderr.fileno())
    readNull.close()
    writeNull.close()
    
    port = 8894
    host = "0.0.0.0"
    server = Server(port,host)
    server.server_process()
    
#upgrade client

def upgrade_client_update(cmd,port):
    if cmd[:-1] == 'update' and len(cmd) != 7:
        print 'cmd error'
        sys.exit(1)

    fd = open('/etc/clumgt.config')
    for line in fd:
        hostname,ip = line.split(' ')
        clientfd = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        if ip[-1] == '\n':
            ip = ip[:-1]
        ret,_ = commands.getstatusoutput("ping "+ip+" 1")
        if ret != 0:
            print ip+" ping fail"
            continue
        try:
            clientfd.connect((ip,port))
        except:
            print ip+" link fail"
            continue
        clientfd.sendall(cmd)

def upgrade_client():
    port = 8894
    cmd = sys.argv[1]
    cmdList = ('exit','starton','startoff','get')
    if len(sys.argv) != 2:
        print 'param error'
        sys.exit(1)
        
    if cmd not in cmdList and cmd[0] != '/' and cmd[:-1] != 'update':
        print 'cmd error'
        sys.exit(1)
        
    if cmd[:-1] == 'update':
        upgrade_client_update(cmd,port)
        return
  
    clientfd = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    try:
        clientfd.connect(('127.0.0.1',port))
    except:
        print "link fail"
        sys.exit(1)
        
    clientfd.sendall(cmd)    
    if cmd == 'get':
        sys.stdout.write(clientfd.recv(2048))
    

if __name__ == '__main__':
    fileName = sys.argv[0]
    ret,isRun = commands.getstatusoutput("ps -ef |grep "+fileName+"|egrep -v 'grep|sh -c|cm_multi_exec'|wc -l")

    if int(isRun) > 1 and len(sys.argv) > 1:
        upgrade_client()
    elif int(isRun) <=1 and len(sys.argv) == 1:
        upgrade_server()
    elif int(isRun) >1 and len(sys.argv) == 1:
        print "server is runing"
        sys.exit(1)
    else:
        print "server is not runing"
        sys.exit(1)


