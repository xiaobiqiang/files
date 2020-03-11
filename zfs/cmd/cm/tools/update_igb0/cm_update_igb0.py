#!/usr/bin/python

import os
import commands
import sys
import ConfigParser

def update_cluster_ini(ip):
    (status,old_ip) = commands.getstatusoutput("cat /var/cm/data/cm_cluster.ini|grep 'ip ='")
    ret = os.system("sed \"s/"+old_ip+"/ip = "+ip+"/g\" /var/cm/data/cm_cluster.ini > /var/cm/data/cm_cluster.ini_bak")
    ret = os.system("mv /var/cm/data/cm_cluster.ini_bak /var/cm/data/cm_cluster.ini")
	
def update_hostname_igb0(ip):
    (status,old_ip) = commands.getstatusoutput("head -n 1 /etc/hostname.igb0")
    ret = os.system("sed \"s/"+old_ip+"/"+ip+"/g\" /etc/hostname.igb0 > /etc/hostname.igb0_bak")
    ret = os.system("mv /etc/hostname.igb0_bak /etc/hostname.igb0")
    

def update_node_db(ip,local_ip):
    ip_list = ip.split('.')
    node_id = int(ip_list[2])<<9 | int(ip_list[3])
    old_ip = local_ip
    ret = os.system("sqlite3 /var/cm/data/cm_node.db \"UPDATE record_t  set id="+str(node_id)+",ipaddr='"+ip+"'  WHERE ipaddr='"+old_ip+"'\"")
	#print "sqlite3 /var/cm/data/cm_node.db \"UPDATE record_t  set id="+str(node_id)+",ipaddr="+ip+"  WHERE ipaddr="+old_ip+"\""
    return node_id

def update_node_db_bak(config):
    node_list = config.options("node")
    for node_df in node_list:
        node = node_df[5:]
        ip = config.get("node",node_df)
        ip_list = ip.split('.')
        node_id = int(ip_list[2])<<9 | int(ip_list[3])
        (ret,local_ip) = commands.getstatusoutput("cat /etc/clumgt.config | grep "+node+"|awk '{print $2}'")
        old_ip = local_ip
        ret = os.system("sqlite3 /var/cm/data/cm_node.db \"UPDATE record_t  set id="+str(node_id)+",ipaddr='"+ip+"'  WHERE ipaddr='"+old_ip+"'\"")
	#print "sqlite3 /var/cm/data/cm_node.db \"UPDATE record_t  set id="+str(node_id)+",ipaddr="+ip+"  WHERE ipaddr="+old_ip+"\""
    return node_id	
	
def update_rpc_port_conf(ip):
	ret = os.system("echo 'rpc_port=igb0;' > /etc/fsgroup/rpc_port.conf")
	ret = os.system("echo 'rpc_addr="+ip+";' >> /etc/fsgroup/rpc_port.conf")

def get_local_ip():
    (ret,cut) =  commands.getstatusoutput("ifconfig -a| grep -n 'igb0: '|awk -F':' '{print $1}'")
    cut = int(cut) + 1 
    (ret,local_ip) = commands.getstatusoutput("ifconfig -a | sed -n '"+str(cut)+"p'|awk '{print $2}'")
    return local_ip

def update_igb0(ip,local_ip):
    ret = os.system("guictl disable")
    ret = os.system("svcadm disable ceres_cm")

    #update_cluster_ini(ip)
    update_hostname_igb0(ip)
    update_rpc_port_conf(ip)
    
    ret = os.system("svcadm enable ceres_cm")
    ret = os.system("guictl enable")

def check_clumgt_config():
    cut = ""
    (ret,cut) = commands.getstatusoutput("cat /etc/clumgt_cmdlist.config |grep cm_update_igb0|wc -l")
    if int(cut) == 0:
        ret = os.system("echo cm_update_igb0.py >> /etc/clumgt_cmdlist.config")
	
if __name__ == '__main__':
    ip = ""
    file_name = ""
    hostname = ""
    local_ip = ""
    ret = ""
    config = ""
	
    (ret,hostname) = commands.getstatusoutput("hostname")
    local_ip = get_local_ip()
        
    if len(sys.argv) != 2 : 
        print "param error!"
        sys.exit(1)
    #check_clumgt_config()
    if os.path.isfile(sys.argv[1]):
        file_name = sys.argv[1]
        config = ConfigParser.ConfigParser()
        config.read(file_name)
        node_ip = config.get("node","node_"+hostname)
        cluster_ip = config.get("cluster","clu_ip")
        print "update "+hostname+" igb0 ip..."
        update_node_db_bak(config)
        update_igb0(node_ip,local_ip)
        update_cluster_ini(cluster_ip)

    else:
        print "error:is not ini file"
		
		
		
		
		
		
		