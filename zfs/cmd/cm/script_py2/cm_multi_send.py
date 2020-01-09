#!/bin/python

import socket
import sys
import platform
import ConfigParser
import os
import commands

def show_process(hostname,file_size,has_sent):
    i = int(float(has_sent)/float(file_size)*100)/2
    a = hostname+"  |"+"#"*i+" "*(50-i)+"| "+str(i*2)+"%"
    sys.stdout.write("\r%s" %a)
    sys.stdout.flush()

"""
def send_file(file_dir,clientfd,has_sent,file_size):
    version = platform.python_version()
    try:
        version = version.split('.')
        if int(version[1]) < 5:
            file_fd = open(file_dir,"rb")
            while has_sent < file_size:
                data = file_fd.read(2048)
                clientfd.sendall(data)
                has_sent += len(data)
                #show_process(hostname,file_size,has_sent)
            file_fd.close()
            return
        else:
            with open(file_dir,"rb") as file_fd:
                for line in file_fd:
                    clientfd.send(line)
    except:
        pass
"""
def send_file(file_dir,clientfd,has_sent,file_size):
    try:
        file_fd = open(file_dir,"rb")
        while has_sent < file_size:
            data = file_fd.read(2048)
            clientfd.sendall(data)
            has_sent += len(data)
            #show_process(hostname,file_size,has_sent)
        file_fd.close()
        return
    except:
        pass


def recv_data(clientfd):
    result = ""
    recvlen = 0
    splitcut = 0
    
    data = clientfd.recv(2048)
    for c in data:
        if c == '$':
            break 
        splitcut+=1

    result += data[splitcut+1:]
    recvlen += len(result)
    while recvlen < int(data[0:splitcut]):
        result += clientfd.recv(2048)
        recvlen += 2048
    return result.decode('utf-8').encode('gbk')
    
def connect_server(ip,port,hostname,load):
    ret = 0
    clientfd = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    system_name = platform.system()
    if system_name == 'Linux':
        ret,_ = commands.getstatusoutput("ping "+ip+" -c 1")
    else:
        ret,_ = commands.getstatusoutput("ping "+ip+" 1")
    if ret != 0:
        sys.stderr.write(hostname+" ping fail\n")
        return
    try:
        clientfd.connect((ip,port))
    except:
        sys.stderr.write(hostname+" link fail\n")
        sys.exit(1)
    clientfd.sendall(load)
    single_is_dir = clientfd.recv(32)
    if "error" in single_is_dir:
        clientfd.recv(2048)
        sys.stderr.write("error! "+hostname+" dir not exist\n")
        sys.exit(1)
    
    return clientfd

def handle_req_single(argv,hostmap,port):
    has_sent = 0
    hostname = argv[1]
    file_dir = argv[2]
    load_dir = argv[3]

    if not os.path.isfile(file_dir):
        sys.stderr.write("error! file is not exist\n")
        sys.exit(1)

    file_name = os.path.basename(file_dir)
    file_size = os.stat(file_dir).st_size
    load = "load|"+file_name+"|"+load_dir+"|"+str(file_size)+'$'
    ip = hostmap[hostname]
    clientfd = connect_server(ip,port,hostname,load)
    send_file(file_dir,clientfd,has_sent,file_size)
    result = recv_data(clientfd)
    clientfd.close()
    print result

def handle_req_broadcast(argv,hostmap,port):
    file_dir = argv[1]
    load_dir = argv[2]
    ret,localhost = commands.getstatusoutput("hostname")
    
    if not os.path.isfile(file_dir):
        sys.stderr.write("error! file is not exist\n")
        sys.exit(1)    

    file_name = os.path.basename(file_dir)
    file_size = os.stat(file_dir).st_size
    load = "load|"+file_name+"|"+load_dir+"|"+str(file_size)+'$'  
    for hostname in hostmap:
        len_dir = len(load_dir)
        if localhost == hostname:
            if os.path.abspath(file_dir)[0:len_dir] == load_dir and len(load_dir)-len_dir <=1:
                continue
        has_sent = 0
        ip = hostmap[hostname]
        clientfd = connect_server(ip,port,hostname,load)
        send_file(file_dir,clientfd,has_sent,file_size)
        result = recv_data(clientfd)
        clientfd.close()
        print result

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
            hostname,ip = line.split(' ')
            hostmap[hostname] = ip.strip()
        fd.close()

if __name__ == '__main__':
    port = 7795
    hostmap = {}

    get_hostmap(hostmap)

    if len(sys.argv) == 4:
        if sys.argv[1] not in hostmap:
            sys.stderr.write("param error,hostname not in /etc/clumgt.config\n")
            sys.stderr.write("please input: "+sys.argv[0]+" hostname <src> <dst>\n")
            sys.exit(1)
        handle_req_single(sys.argv,hostmap,port)
    elif len(sys.argv) == 3:
        handle_req_broadcast(sys.argv,hostmap,port)
    else:
        sys.stderr.write("param error\n")
        sys.exit(1)
