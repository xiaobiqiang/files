#!/bin/bash

source $(dirname `readlink -f $0`)/'inspect_common.sh'

#==============================================================================
###状态是否正常
function clusterd_status()
{
    local result=$INSPECT_OK
    #开始
    INSPECT_START ${FUNCNAME}
    
    if [ `svcs stmf|grep online|wc -l` -eq 0 ]; then
        echo "status :    offline"
        result=$INSPECT_FAIL
    else
        echo "status :    online"
    fi
    
    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#==============================================================================
###查看集群中节点的状态
function clusterd_node()
{
    local result=$INSPECT_OK
    local localhost=`hostname`
    local arrays=($(zfs clustersan list-host -sfp|egrep 'hostname|link'|awk 'NR>1{print}'|awk -F':' '{print $2}'))
    local cols=${#arrays[@]}
    #开始
    INSPECT_START ${FUNCNAME}

    echo "localhost:$localhost"
    for (( i=0; i<$cols; i=i+2 ))
    do
        local j=i
        local name=${arrays[j]}
        local state=${arrays[((j=$j+1))]}
        if [ $state != "up" ]; then
            result=$INSPECT_FAIL
        fi
        echo "hostname:$name    State:$state"
    done

    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#==============================================================================
###查看集群中通信端口的状态
function clusterd_target()
{
    local result=$INSPECT_OK
    local target=`zfs clustersan list-target -v|grep Target`
    #开始
    INSPECT_START ${FUNCNAME}

    zfs clustersan list-target -v|grep Target
    local arrays=($(zfs clustersan list-target -v|egrep 'session:|link state'|awk -F':' '{print $2}'))
    local cols=${#arrays[@]}
    for (( i=0; i<$cols; i=i+2 ))
    do
        local j=i
        local session=${arrays[j]}
        local state=${arrays[((j=$j+1))]}
        if [ $state != "up" ]; then
            result=$INSPECT_FAIL
        fi
        echo "session:$session    State:$state"
    done

    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#==============================================================================
###查看有无stmf_task_alloc日志
function clusterd_log()
{
    local result=$INSPECT_OK

    #开始
    INSPECT_START ${FUNCNAME}

    cat /var/adm/messages| grep stmf_task_alloc

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