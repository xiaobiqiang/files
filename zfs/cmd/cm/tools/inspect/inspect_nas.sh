#!/bin/bash

source $(dirname `readlink -f $0`)/'inspect_common.sh'

#内部函数
#解析集群nas
#Multiclusstatus up
#Groupname cluser
#Masternodeid 1
#Master2nodeid 0
#Master3nodeid 0
#Master4nodeid 0
#Groupnumber 2
#Fsname poola/nas
#Nodetype master
#Spaid 53a05d7c4c511166
#Nodeid 1
#Availsize 500G
#Usedsize 56.5M
#Loadios 0
#Status online

function inter_zfs_multiclus()
{
    zfs multiclus -v |sed 's/ //g' |grep '|' \
        |awk -F'|' 'BEGIN{a="-";b="-";c="-";d="-";e="-";f="-"}
        $2=="Multiclusstatus"{a=$3;continue}
        $2=="Groupname"{b=$3;continue}
        $2=="Fsname"{c=$3;continue}
        $2=="Nodetype"{d=$3;continue}
        $2=="Spaid"{e=$3;continue}
        $2=="Nodeid"{f=$3;continue}
        $2=="Status"{print a" "b" "c" "d" "e" "f" "$3}'
    return 0 
}

#==============================================================================
###检查集群NAS状态
function nas_cluster_status()
{
    local result=$INSPECT_OK
    #开始
    INSPECT_START ${FUNCNAME}
    
    #执行相关检查，并设置巡检结果
    local data=`zfs multiclus -v |egrep "down|offline" `
    if [ "X$data" != "X" ]; then
        result=$INSPECT_FAIL
    fi
    inter_zfs_multiclus
    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result
}
#==============================================================================
###检查各节点集群信息是否一致
function nas_check_data()
{
    local result=$INSPECT_OK
    #开始
    INSPECT_START ${FUNCNAME}
    local cnt=`cm_multi_exec 'zfs multiclus -v |md5sum'|awk 'BEGIN{a="";c=0}$1!=a{a=$1;c++}END{print c}'`
    if [ $cnt -gt 1 ]; then
        echo "zfs multiclus -v not the same"
        result=$INSPECT_FAIL
    fi
    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result
}
#==============================================================================
###检查各NAS信息是否正确
function nas_check_master()
{
    local result=$INSPECT_OK
    local tmpfile='/tmp/nas_check_master.flag'
    local nid=`hostid`
    nid=$((16#$nid))
    
    INSPECT_START ${FUNCNAME}
    rm -f $tmpfile
    inter_zfs_multiclus |while read line
    do
        #up cluser poola/nas master 53a05d7c4c511166 1 online
        #up cluser poolb/nas slave 7f2a2fefca92602b 2 online
        #up limn poolc/test master e9bbc993c5d3f39 1 offline
        local info=($line)
        if [ ${info[5]} -ne $nid ]; then
            continue
        fi
        if [ "X${info[6]}" != "online" ]; then
            continue
        fi
        local masterval=`zfs get master ${info[2]} |sed 1d |awk '{print $3}'`
        local groupval=`zfs get group ${info[2]} |sed 1d |awk '{print $3}'`
        echo "cluster:${info[1]} type:{info[3]} master:$masterval group:$groupval"
        if [ "X{info[3]}" == "Xmaster" ]; then
            if [ "X$state" != "Xon" ]; then
                touch $tmpfile
            fi
        else
            if [ "X$state" == "Xon" ]; then
                touch $tmpfile
            fi
        fi
        if [ "X$groupval" != "Xon" ]; then
            touch $tmpfile
        fi
        #检测浮动IP
        zfs get all ${info[2]} |grep 'failover:' |awk '{print $3}' \
            |awk -F',' '{print $1}' |while read failoverip
        do
            #检测是否设置
            local isset=`ifconfig -a |grep -w "$failoverip"`
            if [ "X$isset" == "X" ]; then
                echo "nas ${info[2]} failoverip:$failoverip not set"
                touch $tmpfile
                continue
            fi
            ping $failoverip 1
            isset=$?
            if [ $isset -ne 0 ]; then
                touch $tmpfile
            fi
        done
    done
    if [ -f $tmpfile ]; then
        rm -f $tmpfile
        result=$INSPECT_FAIL
    fi
    
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#==============================================================================
###检测NAS相关服务是否正常
function nas_check_svc()
{
    local result=$INSPECT_OK
    local tmpfile='/tmp/nas_check_svc.flag'
    INSPECT_START ${FUNCNAME}
    
    svcs -a > $tmpfile
    #检查smb服务状态
#online         17:33:30 svc:/network/smb/client:default
#online         17:33:31 svc:/network/smb/server:default
#online         17:33:31 svc:/network/shares/group:smb

    local cnt=`grep smb $tmpfile |grep '^online' |wc -l`
    if [ $cnt -lt 3 ]; then
        echo "check smb service[$cnt] fail"
        grep smb $tmpfile
        result=$INSPECT_FAIL
    fi
    
    #检查nfs服务状态
#online          9:48:01 svc:/network/nfs/cbd:default
#online          9:48:01 svc:/network/nfs/status:default
#online          9:48:01 svc:/network/nfs/nlockmgr:default
#online          9:48:01 svc:/network/nfs/mapid:default
#online          9:48:02 svc:/network/nfs/client:default
#online          9:48:02 svc:/network/nfs/rquota:default
#online          9:48:02 svc:/network/nfs/server:default
    cnt=`grep nfs $tmpfile |grep '^online' |wc -l`
    if [ $cnt -lt 7 ]; then
        echo "check nfs service[$cnt] fail"
        grep nfs $tmpfile
        result=$INSPECT_FAIL
    fi

    #检查idmap服务状态
#online         17:33:30 svc:/system/idmap:default
    cnt=`grep idmap $tmpfile |grep '^online' |wc -l`
    if [ $cnt -lt 1 ]; then
        echo "check idmap service[$cnt] fail"
        grep idmap $tmpfile
        result=$INSPECT_FAIL
    fi

    #检查rpc服务状态
#online          9:48:01 svc:/network/rpc/bind:default
#online          9:48:02 svc:/network/rpc/gss:default
#online          9:48:02 svc:/network/rpc/rstat:default
#online          9:48:02 svc:/network/rpc/rusers:default
#online          9:48:02 svc:/network/rpc-100235_1/rpc_ticotsord:default
#online          9:48:05 svc:/system/cluster_rpc:default

    cnt=`grep rpc $tmpfile |grep '^online' |wc -l`
    if [ $cnt -lt 6 ]; then
        echo "check rpc service[$cnt] fail"
        grep rpc $tmpfile
        result=$INSPECT_FAIL
    fi
    
    rm -f $tmpfile
    INSPECT_END ${FUNCNAME} $result
    return $result
}
#==============================================================================
###检查NAS日志是否正常
function nas_check_log()
{
    local result=$INSPECT_OK
    local cnt=0
    local chkstrs=('Can not get the mac' 'spa:.*os:.* OFFLINE' 'write operate timeout' 'long fid return')
    local logfile='/var/adm/messages'
    local cntstart=0
    local tmpfile='/tmp/nas_check_log.flag'
    #开始
    INSPECT_START ${FUNCNAME}
    rm -f $tmpfile
    for((index=0;index<${#chkstrs[*]};index++))
    do
        local chkstr=${chkstrs[$index]}
        cnt=`egrep "$chkstr" $logfile |wc -l |awk '{print $1}'`
        if [ $cnt -gt 0 ]; then
            cntstart=1
            if [ $cnt -gt 5 ]; then
                ((cntstart=$cnt-5))
            fi
            echo  "find count[$cnt]:$chkstr"
            egrep "$chkstr" $logfile |sed -n "$cntstart,${cnt}p"
            touch $tmpfile
        fi
    done
    
    if [ -f $tmpfile ]; then
        rm -f $tmpfile
        result=$INSPECT_FAIL
    fi
    
    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result
}
#=============================================================================
#=============================================================================
inspect_main $*
exit $?
#=============================================================================
#=============================================================================
