#!/bin/bash

CM_DISK_DB='/var/cm/data/cm_db_disk.db'
CM_FUNC_PRE='cm_disk_'

function cm_disk_help()
{
    echo "format:"
    echo "$0 local_enid"
    echo "$0 device_getbatch"
    echo "$0 device_count"
    echo "$0 getbatch [enid]"
    echo "$0 count [enid]"
    echo "$0 led <enid> <slotid> <on/off>"
    return 0
}

function cm_disk_local_enid()
{
    local interid=`hostid`
    interid=`printf %d "0x$interid"`
    ((interid=(($interid+1)>>1)*1000))
    echo $interid
    return 0
}

function cm_disk_device_getbatch()
{
    local myenid=`cm_disk_local_enid`
    local isnew=0
    local lastpart=$myenid
    local curpart=0
    ((lastpart=$lastpart/1000))
    sqlite3 $CM_DISK_DB "select distinct enid from record_t order by enid" |while read line
    do
        local enid=$line
        ((enid=$enid-22))
        if [ $enid -lt 1000 ]; then
            ((enid=$enid+$myenid))
        fi
        ((curpart=$enid/1000))
        if [ $curpart -ne $lastpart ]; then
            echo ""
            lastpart=$curpart          
        fi
        printf "$enid "
    done
    echo ""
    return 0
}

function cm_disk_device_count()
{
    sqlite3 $CM_DISK_DB "select count(distinct enid) from record_t"
    return $?
}

function disk_enid_tolocal()
{
    local enid=$1
    ((enid=$enid+22))
    local myenid=`cm_disk_local_enid`
    local curpart=$myenid
    ((curpart=$curpart/1000))
    local inputpart=0
    ((inputpart=$enid/1000))
    if [ $curpart -eq $inputpart ]; then
        ((enid=$enid%1000))
    fi
    echo $enid
    return 0
}

function cm_disk_getbatch()
{
    local enid=$1
    local index=0
    if [ "X$enid" == "X" ]; then
        sqlite3 $CM_DISK_DB "select id, enid, slotid from record_t" |sed 's/|/ /g'
        return $?
    fi
    
    if [ $enid -lt 1000 ]; then
        echo '0'
        return 0
    fi
    enid=`disk_enid_tolocal $enid`    
    sqlite3 $CM_DISK_DB "select id, enid, slotid from record_t where enid=$enid" |sed 's/|/ /g'
    return $?
}

function cm_disk_count()
{
    local enid=$1
    
    if [ "X$enid" == "X" ]; then
        sqlite3 $CM_DISK_DB "select count(id) from record_t"
        return $?
    fi
    
    if [ $enid -lt 1000 ]; then
        echo '0'
        return 0
    fi
    enid=`disk_enid_tolocal $enid`    
    sqlite3 $CM_DISK_DB "select count(id) from record_t where enid=$enid"
    return $?
}

function cm_disk_led()
{
    local enid=$1
    local slotid=$2
    local state=$3
    local localenid=`disk_enid_tolocal $enid`
    local diskid=`sqlite3 $CM_DISK_DB "select id from record_t where enid=$localenid and slotid=$slotid"`
    
    if [ "X$diskid" == "X" ]; then
        echo "no disk!"
        return 1
    fi
    
    if [ "X$state" == "Xon" ]; then
        state='fault'
    else
        state='normal'
    fi
    ((localenid=$enid/1000))
    local ipaddr=`sqlite3 /var/cm/data/cm_node.db "select ipaddr from record_t where sbbid=$localenid limit 1"`
    if [ "X$ipaddr" == "X" ]; then
        echo "the host of $enid not found!"
        return 1
    fi
    local cmd="disk led -d /dev/rdsk/$diskid -o $state"
    echo "[$ipaddr]$cmd"
    ceres_exec $ipaddr "$cmd"
    return 0
}

if [ "X$1" == "X" ]; then
    ${CM_FUNC_PRE}help
    exit $?
else
    ${CM_FUNC_PRE}$*
    exit $?
fi
