#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

function cm_port_local_getbatch()
{
    local mid=`cm_get_localid`
    stmfadm list-target -v |sed 's/ //g' \
        |egrep "Target|OperationalStatus|ProviderName|Protocol|Sessions" \
        |xargs -n5 |grep -v pppt \
        |sed 's/Target://g' \
        |sed 's/OperationalStatus://g' \
        |sed 's/ProviderName://g' \
        |sed 's/Protocol://g' \
        |sed 's/Sessions://g' \
        |while read line
    do
        echo "$line $mid"
    done
        
    return 0
}

function cm_port_initiator_getbatch()
{
    local portname=$2
    local nid=$1
    local nodename=`sqlite3 $CM_NODE_DB_FILE "SELECT name FROM record_t WHERE id=$nid"`
    if [ "X$nodename" == "X" ]; then
        return 0
    fi
    cm_multi_exec $nodename "stmfadm list-target -v $portname" |grep -w Initiator |awk '{print $2}'
    return 0
}

function cm_port_initiator_count()
{
    local portname=$2
    local nid=$1
    local nodename=`sqlite3 $CM_NODE_DB_FILE "SELECT name FROM record_t WHERE id=$nid"`
    if [ "X$nodename" == "X" ]; then
        echo "0"
        return 0
    fi
    cm_multi_exec $nodename "stmfadm list-target -v $portname" |grep -w Initiator |wc -l
    return 0
}


function cm_port_getbatch()
{
    cm_multi_exec "/var/cm/script/cm_cnm_port.sh local_getbatch"
    return 0
}

function cm_port_count()
{
    stmfadm list-target 2>/dev/null |wc -l
    return 0
}

function cm_port_group_getbatch()
{
    local type=$1
    stmfadm list-${type} |awk '{print $3}'
    return 0
}

function cm_port_group_count()
{
    local type=$1
    stmfadm list-${type} |wc -l
    return 0
}

function cm_port_group_member_getbatch()
{
    local type=$1
    local gname=$2
    stmfadm list-${type} -v $gname |sed 1d |awk '{print $2}'
    return 0
}
function cm_port_group_member_count()
{
    local type=$1
    local gname=$2
    stmfadm list-${type} -v $gname |sed 1d |wc -l
    return 0
}

cm_port_$*
exit $?


