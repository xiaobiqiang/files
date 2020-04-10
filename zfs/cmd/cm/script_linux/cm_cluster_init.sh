#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'
#在/usr/sbin/cluster_init.sh调用,根据cm_cluster.ini文件配置
CM_CLUSTER_INI='/var/cm/data/cm_cluster.ini'
CM_CLUSTER_NODE_INI='/var/cm/data/cm_cluster_rdma.ini'

function cm_cluster_san_rdma_set()
{
    local iRet=$CM_OK
    if [ ! -f ${CM_CLUSTER_NODE_INI} ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] config file not exist"
        return $CM_OK
    fi
    local nic=`grep 'nic' $CM_CLUSTER_INI |grep rdma_rpc`
    if [ "X$nic" == "X" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] not config"
        return $CM_OK
    fi
    local myname=`hostname`
    cat $CM_CLUSTER_NODE_INI|sed "/${myname}_/d" |while read line
    do
        local info=(`echo "$line" |sed 's/[_=\,]/ /g'`)
        local len=${#info[@]}
        for((i=2;i<$len;i++))
        do
            local ip=${info[$i]}
            zfs clustersan rpcto hostname=${info[0]} hostid=${info[1]} ip=$ip
            iRet=$?
            CM_LOG "[${FUNCNAME}:${LINENO}]rdma_rpc ${info[0]} ${info[1]} $ip iRet=$iRet"
        done
    done
    return $CM_OK
}

function cm_cluster_getmirrorid()
{
    local mirrorid=0
    local cfgfile='/var/cm/data/node_config.ini'
    if [ -f $cfgfile ]; then
        mirrorid=`grep '^mirrorid' $cfgfile |sed 's/ //g' |awk -F'=' '{print $2}'`
        if [ "X$mirrorid" != "X" ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}]get $mirrorid from file"
            echo $mirrorid
            return 0
        fi
    fi
    local myhostid=`hostid`
    myhostid=`printf %d "0x$myhostid"`
    CM_LOG "[${FUNCNAME}:${LINENO}]hostid=$myhostid"
    ((mirrorid=$myhostid&1))
    if [ $mirrorid -eq 1 ]; then
        ((mirrorid=$myhostid+1))
    else
        ((mirrorid=$myhostid-1))
    fi
    CM_LOG "[${FUNCNAME}:${LINENO}]get $mirrorid from hostid"
    echo $mirrorid
    return 0
}

function cm_cluster_san_init()
{
    local iRet=$CM_OK
    CM_LOG "[${FUNCNAME}:${LINENO}]start"
    if [ ! -f ${CM_CLUSTER_INI} ]; then
        #如果配置文件不存在,说明当前没有配置集群,直接退出
        CM_LOG "[${FUNCNAME}:${LINENO}] not config"
        return $CM_OK
    fi
    local crtime=`egrep "^time " $CM_CLUSTER_INI |awk '{print $3}'`
    if [ "X$crtime" == "X" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] no time"
        #集群未配置，退出
        return $CM_OK
    fi
    
    local val=`egrep "^name " $CM_CLUSTER_INI |awk '{print $3}'`
    if [ "X$val" == "X" ]; then
        #集群名称未配置，退出
        CM_LOG "[${FUNCNAME}:${LINENO}] no name"
        return $CM_FAIL
    fi
    
    #设置集群名称
    zfs clustersan enable -n $val
    iRet=$?
    CM_LOG "[${FUNCNAME}:${LINENO}]name $val iRet=$iRet"
    sleep 5
    
    #计算对端节点ID
    local mirrorid=`cm_cluster_getmirrorid`
    
    #设置对端节点ID
    zfs mirror -e $mirrorid 1>/dev/null 2>/dev/null
    iRet=$?
    CM_LOG "[${FUNCNAME}:${LINENO}]mirror $mirrorid iRet=$iRet"
    
    #配置集群通信网口
    local nics=(`egrep "^nic " $CM_CLUSTER_INI|awk -F'=' '{print $2}' |sed 's/\,/ /g'`)
    local rdmaset=`egrep "^nic " $CM_CLUSTER_INI |grep rdma_rpc`
    if [ "X$rdmaset" == "X" ]; then
        local index=0
        local cnt=${#nics[*]}
        local ostype=`cm_os_type_get`
        for((index=0;index<$cnt;index++))
        do
            # -p ${priority[$index]}
            if [ "X${nics[$index]}" != "Xrdma_rpc" ] && [ $ostype -eq ${CM_OS_TYPE_ILLUMOS} ]; then
                dladm set-linkprop -p mtu=9000 ${nics[$index]}
            fi
            ifconfig ${nics[$index]} mtu 9000
            zfs clustersan enable -l ${nics[$index]}
            
            iRet=$?
            CM_LOG "[${FUNCNAME}:${LINENO}]nic ${nics[$index]} iRet=$iRet"
        done
    else
        zfs clustersan enable -l rdma_rpc
        iRet=$?
        CM_LOG "[${FUNCNAME}:${LINENO}]nic rdma_rpc iRet=$iRet"
        cm_cluster_san_rdma_set
    fi    
    
    #配置ipmi
    val=`egrep "^ipmi " $CM_CLUSTER_INI |awk '{print $3}'`
    if [ "X$val" == "XON" ]; then
        val='on'
    else
        val='off'
    fi
    zfs clustersan set ipmi=$val
    iRet=$?
    CM_LOG "[${FUNCNAME}:${LINENO}]ipmi $val iRet=$iRet"
    
    #配置failover
    val=`egrep "^failover " $CM_CLUSTER_INI |awk '{print $3}'`
    if [ "X$val" != "X" ]; then
        zfs clustersan set failover=$val
        iRet=$?
        CM_LOG "[${FUNCNAME}:${LINENO}]failover $val iRet=$iRet"
    fi
    
    zfs multiclus -e
    iRet=$?
    CM_LOG "[${FUNCNAME}:${LINENO}]clusternas iRet=$iRet"
    return $CM_OK
}
/var/cm/script/cm_shell_exec.sh cm_cluster_nas_check
cm_cluster_san_init
exit $?
