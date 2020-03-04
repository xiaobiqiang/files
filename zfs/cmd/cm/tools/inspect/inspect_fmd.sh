#!/bin/bash

source $(dirname `readlink -f $0`)/'inspect_common.sh'

#==============================================================================
###列举fmd加载模块
function fmd_config()
{
    local result=$INSPECT_OK
    #开始
    INSPECT_START ${FUNCNAME}
    
    fmadm config
    
    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#==============================================================================
###列举fmd faulty记录 
function fmd_faulty()
{
    local result=$INSPECT_OK
    #开始
    INSPECT_START ${FUNCNAME}
    
    fmadm faulty -s|awk  'NR>3{print}'
    
    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result    
}

#==============================================================================
###列举fmd error
function fmd_error()
{
    local result=$INSPECT_OK
    local cut=`fmdump -ev|wc -l`
    #开始
    INSPECT_START ${FUNCNAME}
    if [ $cut -le 100 ]; then
        fmdump -ev
    else
        ((cut=$cut-100))
        fmdump -ev | awk 'NR>'$cut'{print}'		
    fi
    
    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result    
}

#==============================================================================
###列举所有磁盘
function fmd_disk()
{
    local result=$INSPECT_OK
    #开始
    INSPECT_START ${FUNCNAME}
    
    #disk list #没有意义
    
    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result    
}

#==============================================================================
###列举所有链路
function fmd_mpathadm()
{
    local result=$INSPECT_OK
    local mpathadm=`mpathadm list lu|grep /dev/rdsk`
    local tmpfile='/tmp/nas_check_master.flag'
    #开始
    INSPECT_START ${FUNCNAME}
    echo "all mpathadm:"
    mpathadm list lu|grep /dev/rdsk
	
    echo ""
    echo "abnormal mpathadm:"
    disk list|grep /dev/|awk '{print $2}'|while read line
    do 
        if [ `echo $mpathadm|grep $line|wc -l` -eq 0 ]; then
            touch $tmpfile
            echo $line
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