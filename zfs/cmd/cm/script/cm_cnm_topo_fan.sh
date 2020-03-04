#!/bin/bash

cm_topo_ssh1='$1'
cm_topo_ssh2='$2'
cm_topo_ssh4='$4'

function get_ips_from_sbbid()
{
    local sbbid=$1
    local ret=`sqlite3 /var/cm/data/cm_node.db "select ipaddr from record_t WHERE sbbid=$sbbid" 2>>/dev/null`
    if [ "$ret"x == ""x ];then
        ret="none"
    fi
    echo $ret
}

function get_nid_from_ip()
{
    local ip=$1
    local ret=`sqlite3 /var/cm/data/cm_node.db "select id from record_t WHERE ipaddr=\"$ip\"" 2>>/dev/null`
    if [ "$ret"x == ""x ];then
        ret="none"
    fi
    echo $ret
}

ips=$(get_ips_from_sbbid $1)
if [ "$ips"x == "none"x ];then
    exit
fi

for ip in ${ips[@]}
do
    nid=$(get_nid_from_ip $ip)
    timeout 5 ceres_exec $ip "cat /tmp/sensor.txt 2>/dev/null |grep RPM|awk -F'|' '{print $cm_topo_ssh1 $cm_topo_ssh2 $cm_topo_ssh4 \"$nid\"}'"
done

exit


