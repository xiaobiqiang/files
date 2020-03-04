#!/bin/python

import socket
import sys
import commands
import ConfigParser
import platform


timeout = 10

def recv_data(clientfd):
    result = ""
    recvlen = 0
    splitcut = 0
    system_name = platform.system()
    # recv : ret|len$data
    data = clientfd.recv(2048)
    for c in data:
        if c == '$':
            break 
        splitcut+=1

    ret,datalen = data[0:splitcut].split('|')
    ret,datalen = int(ret),int(datalen)
        
    result += data[splitcut+1:]
    if len(result) != 0:
        if system_name.startswith('Windows'):
            print result.decode('utf-8').encode('gbk'),
        else:
            sys.stdout.write(result)

    recvlen += len(result)
    if system_name.startswith('Windows'):
        while recvlen < datalen:
            print clientfd.recv(2048).decode('utf-8').encode('gbk'),
            recvlen += 2048
    else:
        while recvlen < datalen:
            sys.stdout.write(clientfd.recv(2048))
            recvlen += 2048

    return [ret,recvlen]

def remote_cmd(hostname,ip,port,cmd):
    global timeout
    recv_len = 0
    ret,_ = commands.getstatusoutput("ping "+ip+" -c  1")
    if ret != 0:
        sys.stderr.write(hostname+" ping fail\n")
        return
    
    clientfd = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    clientfd.settimeout(timeout)
    try:
        clientfd.connect((ip,port))
    except:
        sys.stderr.write(hostname+" link fail\n")
        return
    clientfd.sendall(cmd)
    try:
        ret,recv_len = recv_data(clientfd)
    except:
        sys.stderr.write(hostname+" exec timeout\n")
        return
    if recv_len != 0:
        print ""
    clientfd.close()
    return ret
    
    
def handle_req_broadcast(argv,hostmap,port):
    ret = 0
    ret_bak = 0
    result = ""
    cmd = "cmd|"+argv[1]
    for hostname in hostmap:
        ip = hostmap[hostname]
        ret = remote_cmd(hostname,ip,port,cmd)
        if ret != 0:
            ret_bak = ret
    if  ret_bak != 0:       
        sys.exit(1)
        


def handle_req_single(argv,hostmap,port):
    ret = 0
    hostname = argv[1]
    cmd = "cmd|"+argv[2]
    ip = hostmap[hostname]
    ret = remote_cmd(hostname,ip,port,cmd)
    if ret != 0 :
        sys.exit(1)
    
def get_hostmap(hostmap):
    system_name = platform.system()
    if system_name.startswith('Windows'):
        config = ConfigParser.ConfigParser()
        config.read('cm_multi.ini')
        list_hostname = config.options('node')
        for hostname in list_hostname:
            hostmap[hostname] =  config.get('node',hostname)
    else:
        fd = open('/etc/clumgt.config')
        for line in fd:
            if line[:1] == '#' or line[:2] != "df" :
                continue
            hostname,ip = line.split(' ')
            hostmap[hostname] = ip.strip()
        fd.close()

if __name__ == '__main__':
    port = 7795
    hostmap = {}
    
    get_hostmap(hostmap)
    
    if len(sys.argv) == 3:
        if sys.argv[1] not in hostmap:
            sys.stderr.write("param error,hostname not in /etc/clumgt.config\n")
            sys.stderr.write("please input: "+sys.argv[0]+" hostname 'cmd'\n")
            sys.exit(1)
        handle_req_single(sys.argv,hostmap,port)
    elif len(sys.argv) == 2:
        handle_req_broadcast(sys.argv,hostmap,port)
    else:
        sys.stderr.write("param error\n")
        sys.exit(1)


