#!/bin/bash

source $(dirname `readlink -f $0`)/'inspect_common.sh'

#==============================================================================
###状态是否正常
function stmf_status()
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
###查看lu的状态
function stmf_lu()
{
    local result=$INSPECT_OK
    local lu_start="/dev/zvol/rdsk/"
    local len=`echo $lu_start|wc -L`
    #开始
    INSPECT_START ${FUNCNAME}
     
    local lun_arrays=($(stmfadm list-lu -v | egrep  'Alias|State'|awk -F':' '{print $2}'))
    local cols=${#lun_arrays[@]}
    for (( i=0; i<$cols; i=i+2 ))
    do
        local j=i
        local name=${lun_arrays[j]}
        local state=${lun_arrays[((j=$j+1))]}
        echo "Alias:$name    State:$state"
        if [ $state = "transition" ]; then
            result=$INSPECT_FAIL
        fi
    done

    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result    
}

#==============================================================================
###查看target的状态
function stmf_target()
{
    local result=$INSPECT_OK
    #开始
    INSPECT_START ${FUNCNAME}

    local list=`stmfadm list-target |awk -F':' '{print $2}'`
    for line in $list  
    do
        local state=`stmfadm list-target -v $line|grep Operational|awk -F':' '{print $2}'`
        local Sessions=`stmfadm list-target -v $line|grep Sessions|awk -F':' '{print $2}'`
        echo "Target:$line   Status:$state    Sessions:$Sessions"
        if [ $state = "offline" ]; then
            result=$INSPECT_FAIL
        fi
    done

    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result       
}

#==============================================================================
###查看LUN映射信息
function stmf_lunmap()
{
    local result=$INSPECT_OK
    #开始
    INSPECT_START ${FUNCNAME}

    lunadm list-lunmap -v

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