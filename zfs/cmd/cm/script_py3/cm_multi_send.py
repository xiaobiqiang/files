#!/bin/python

import socket
import sys
import platform
import os
import subprocess

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
    
    data = clientfd.recv(32).decode('utf-8')
    for c in data:
        if c == '$':
            break 
        splitcut+=1

    result += data[splitcut+1:]
    recvlen += len(result)
    while recvlen < int(data[0:splitcut]):
        result += clientfd.recv(2048).decode('utf-8')
        recvlen += 2048
    return result

def handle_req_single(argv,hostmap,port):
    has_sent = 0
    clientfd = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    hostname = argv[1]
    file_dir = argv[2]
    load_dir = argv[3]

    if not os.path.isfile(file_dir):
        print("error! file is not exist")
        sys.exit()

    file_name = os.path.basename(file_dir)
    file_size = os.stat(file_dir).st_size
    load = "load|"+file_name+"|"+load_dir+"|"+str(file_size)+'$'
    ip = hostmap[hostname]
    ret,_ = subprocess.getstatusoutput("ping "+ip+" 1")
    if int(ret) != 0:
        print(hostname+" ping fail")
        sys.exit(1)
    try:
        clientfd.connect((ip,port))
    except:
        print(hostname+" link fail")
        sys.exit(1)
    clientfd.sendall(load.encode('utf-8'))
    single_is_dir = clientfd.recv(32).decode('utf-8')
    if "error" in single_is_dir:
        result = recv_data(clientfd)
        print(result)
        return
    send_file(file_dir,clientfd,has_sent,file_size)
    result = recv_data(clientfd)
    clientfd.close()
    print(result)

def handle_req_broadcast(argv,hostmap,port):
    file_dir = argv[1]
    load_dir = argv[2]
    
    if not os.path.isfile(file_dir):
        print("error! file is not exist")
        sys.exit(1)    

    file_name = os.path.basename(file_dir)
    file_size = os.stat(file_dir).st_size
    load = "load|"+file_name+"|"+load_dir+"|"+str(file_size)+'$'
    child_fd = subprocess.Popen('hostname',shell=True,stdout=subprocess.PIPE)
    localhost = child_fd.stdout.read().decode('utf-8').strip()	
    for hostname in hostmap:
        len_dir = len(load_dir)
        if localhost == hostname:
            if os.path.abspath(file_dir)[0:len_dir] == load_dir and len(load_dir)-len_dir <=1:
                continue
        has_sent = 0
        clientfd = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
        ip = hostmap[hostname]
        ret,_ = subprocess.getstatusoutput("ping "+ip+" 1")
        if int(ret) != 0:
            print(hostname+" ping fail")
            return
        try:
            clientfd.connect((ip,port))
        except:
            print(hostname+" link fail")
            continue
        clientfd.sendall(load.encode('utf-8'))
        single_is_dir = clientfd.recv(32).decode('utf-8')
        if "error" in single_is_dir:
            result = recv_data(clientfd)
            print(result)
            continue

        send_file(file_dir,clientfd,has_sent,file_size)
        result = recv_data(clientfd)
        clientfd.close()
        print(result)

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

    if len(sys.argv) == 4:
        if sys.argv[1] not in hostmap:
            print("param error,hostname not in config")
            print("please input: "+sys.argv[0]+" hostname <src> <dst>")
            sys.exit(1)
        handle_req_single(sys.argv,hostmap,port)
    elif len(sys.argv) == 3:
        handle_req_broadcast(sys.argv,hostmap,port)
    else:
        print("param error")
        sys.exit(1)
