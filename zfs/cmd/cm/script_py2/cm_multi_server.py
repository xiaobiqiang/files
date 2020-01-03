#!/bin/python

import socket
import select
import commands
import os
import sys
import threading
import time

class Server:
    def __init__(self,host,port,timeout):
        self.serfd = ""
        self.host = host
        self.port = port
        self.timeout = timeout
        self.inputs = []
        self.outmap = {}
        self.maxlink = ""
        self.loop = True
        self.threadlock = threading.Lock()
        self.threads = []
        self.log_fd = open('/var/cm/log/cm_multi.log','a')

    def cm_multi_log(self,type,recv):
        now = time.asctime(time.localtime(time.time()))
        log_dir = "/var/cm/log/cm_multi.log"
        try:
            if type == "exec":
                log = now+" operation:"+type+" cmd:"+recv+"\n"
                self.log_fd.write(log)
                self.log_fd.flush()
                print log
            elif type == "load":
                log = now+" operation:"+type+" file name:"+recv+"\n"
                self.log_fd.write(log)
                self.log_fd.flush()
                print log
        except:
            return        

    def send_result(self,sock,result):
        sendlen = len(result)
        sock.sendall(str(sendlen)+"$")
        sock.sendall(result)  

    def server_file_load(self,data,client):
        split_cut = 0
        has_recv = 0
        for c in data:
            if c == '$':
                break
            split_cut+=1
        ret,hostname = commands.getstatusoutput("hostname")
        type,file_name,load_dir,file_size = data[0:split_cut].split('|')

        if not os.path.isdir(load_dir):
            client.sendall("error")
            return hostname+" fail: dir is not exist"
        client.sendall("continue")
        file_name = file_name.decode('gbk').encode('utf-8')
        try:
            f = open(load_dir+"/"+file_name,"wr")
            f.write(data[split_cut+1:])
            has_recv = len(data[split_cut+1:])
            while has_recv < int(file_size):
                recv_data = client.recv(2048)
                has_recv += len(recv_data)
                f.write(recv_data)    
            f.close()
        except:
            f.close()
            return hostname+" fail"
        self.cm_multi_log('load',file_name)
        return hostname+" "+file_name+" transfer success"

    def server_handle(self,client):
        data = client.recv(2048)
        result = ""
        if not data:
            client.close()
            return
        if "cmd" == data[0:3]:
            cmds = data.split('|')
            cmd = ""
            for i in range(1,len(cmds)):
                if i != len(cmds)-1:
                    cmd += cmds[i]+'|'
                else:
                    cmd += cmds[i]
            #print cmd
            ret,result = commands.getstatusoutput(cmd)
            self.send_result(client,result)
            self.cm_multi_log('exec',cmd)
            client.shutdown(socket.SHUT_WR)
        elif "load" ==  data[0:4]:
            result = self.server_file_load(data,client)
            self.send_result(client,result)
            client.shutdown(socket.SHUT_WR)
        else:
            self.send_result(client,"none messege")
            client.shutdown(socket.SHUT_WR)
    
    def server_process(self):
        self.serfd = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        self.serfd.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1)
        localaddr = ('',self.port)
        self.serfd.bind(localaddr)
        #self.serfd.setblocking(False)
        self.serfd.listen(100)
        self.inputs = [self.serfd]
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
                        print "link "+str(addr)
                        handle_t = threading.Thread(target=self.server_handle,args=(client,))
                        handle_t.start()                
                    else:
                        self.server_handle(sock)
        self.serfd.close()
            
            

if __name__ == '__main__':

    pid = os.fork()
    if pid:
        sys.exit(0)
    os.umask(0)
    os.setsid()
    c_pid = os.fork()
    if c_pid:
        sys.exit(0)
    os.chdir('/')
    sys.stdout.flush()
    sys.stderr.flush()
        
    read_null = open('/dev/null')
    write_null = open('/dev/null','w')
    os.dup2(read_null.fileno(),sys.stdin.fileno())
    os.dup2(write_null.fileno(),sys.stdout.fileno())
    os.dup2(write_null.fileno(),sys.stderr.fileno())
    read_null.close()
    write_null.close()

    port = 7795
    host = "0.0.0.0"
    server = Server(host,port,10)
    server.server_process()


