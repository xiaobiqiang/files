#!/bin/python

import socket
import sys
import platform
import subprocess

def recv_data(clientfd):
    result = ""
    recvlen = 0
    splitcut = 0
    system_name = platform.system()
    
    data = clientfd.recv(32).decode('utf-8')
    for c in data:
        if c == '$':
            break 
        splitcut+=1

    result += data[splitcut+1:]
    if len(result) != 0:
        if system_name.startswith('Windows'):
            print(result.decode('utf-8').encode('gbk'),)
        else:
            print(result,end='')
    recvlen += len(result)
    
    if system_name.startswith('Windows'):
        while recvlen < int(data[0:splitcut]):
            print(clientfd.recv(2048).decode('utf-8').encode('gbk'))
            recvlen += 2048
    else:
        while recvlen < int(data[0:splitcut]):
            print(clientfd.recv(2048).decode('utf-8'),end='')
            recvlen += 2048			

    return recvlen

def handle_req_broadcast(argv,hostmap,port):
    result = ""
    cmd = "cmd|"+argv[1]
    for hostname in hostmap:
        ip = hostmap[hostname]
        clientfd = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        ret,_ = subprocess.getstatusoutput("ping "+ip+" 1")
        if int(ret) != 0 :
            print(hostname+" ping fail")
            continue
        try:
            clientfd.connect((ip,port))
        except:
            print(hostname+" link fail")
            continue
        clientfd.sendall(cmd.encode('utf-8'))
        recv_data(clientfd)
        clientfd.close()
	
	
def handle_req_single(argv,hostmap,port):
    clientfd = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    hostname = argv[1]
    cmd = "cmd|"+argv[2]
    ip = hostmap[hostname]
    ret,_ = subprocess.getstatusoutput("ping "+ip+" 1")
    if int(ret) != 0:
        print(hostname+" ping fail")
        return
    try:
        clientfd.connect((ip,port))
    except:
        print(hostname+" link fail")
        sys.exit(1)
    clientfd.sendall(cmd.encode('utf-8'))
    recv_data(clientfd)
    clientfd.close()
    
def get_hostmap(hostmap):
    fd = open('/etc/clumgt.config')
    for line in fd:
        hostname,ip = line.split(' ')
        hostmap[hostname] = ip.strip()
    fd.close()

if __name__ == '__main__':
    port = 7795
    hostmap = {}
    
    get_hostmap(hostmap)
    
    if len(sys.argv) == 3:
        if sys.argv[1] not in hostmap:
            print("param error,hostname not in config")
            print("please input: "+sys.argv[0]+" hostname 'cmd'")
            sys.exit(1)
        handle_req_single(sys.argv,hostmap,port)
    elif len(sys.argv) == 2:
        handle_req_broadcast(sys.argv,hostmap,port)
    else:
        print("param error")
        sys.exit(1)


