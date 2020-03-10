#!/bin/bash

source $(dirname `readlink -f $0`)/'inspect_common.sh'

#==============================================================================
###检查存储池状态
function zfs_pool_status()
{
    local result=$INSPECT_OK
    local arrays=($(zpool status|egrep "pool:|state:"|awk -F':' '{print $2}'))
    local cols=${#arrays[@]}
    #开始
    INSPECT_START ${FUNCNAME}
    
    for (( i=0; i<$cols; i=i+2 ))
    do
        local j=i
        local pool=${arrays[j]}
        local state=${arrays[((j=$j+1))]}
        if [ $state != "ONLINE" ]; then
            result=$INSPECT_FAIL
        fi
        echo "pool:$pool    State:$state"
    done        

    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#==============================================================================
###检查zfs数据集
function zfs_list()
{
    local result=$INSPECT_OK

    #开始
    INSPECT_START ${FUNCNAME}
    
    zfs list

    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#==============================================================================
###检查zfs文件系统是否挂载
function zfs_df()
{
    local result=$INSPECT_OK
    local dfh=`df -h`
    local tmpfile='/tmp/nas_check_master.flag'

    #开始
    INSPECT_START ${FUNCNAME}
    
    zfs list -t filesystem|egrep -v NAME|awk '{print $1}'|while read line
    do
        if [ `echo $dfh|grep $line|wc -l` -eq 0 ]; then
            echo "$line is not mount"
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