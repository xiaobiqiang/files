#!/bin/bash

source $(dirname `readlink -f $0`)/'inspect_common.sh'

#==============================================================================
###根目录检查功能
function cm_rootdir()
{
    local result=$INSPECT_OK
    local DEFAULT_DIRRATE=95
    #开始
    INSPECT_START ${FUNCNAME}
    
    #执行相关检查，并设置巡检结果
    df -h |awk '(($6=="/")||($6=="/root")||($6=="/etc")||($6=="/gui")||($6=="/var")){print $6" "$5}' |sed 's/%//g' |while read line
    do
        local info=($line)
        local rate=${line[1]}
        printf "%-10s %10s%%\n" ${line[0]} ${line[1]}
        ((rate=$rate+0))
        if [ $rate -ge ${DEFAULT_DIRRATE} ]; then
            result=$INSPECT_FAIL
        fi
    done
    
    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#==============================================================================
###根目录下core文件检查
function cm_rootcore()
{
    local result=$INSPECT_OK
    local corefile='/core'
    
    INSPECT_START ${FUNCNAME}
    
    if [ -f $corefile ]; then
        echo '::stack'|mdb $corefile
        result=$INSPECT_FAIL
    fi
    
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#==============================================================================
###cron 服务检查
function cm_check_cron()
{
    local result=$INSPECT_OK
    
    INSPECT_START ${FUNCNAME}
    
    #检查服务状态
    local status=`svcs cron |sed -n 2p |awk '{print $1}'`
    if [ "X$status" != "Xonline" ]; then
        echo "cron state: $status"
        result=$INSPECT_FAIL
    fi
    
    #检查配置文件是否存在
    local crontal_cfgfile='/var/spool/cron/crontabs/root'
    if [ ! -f ${crontal_cfgfile} ]; then
        echo "$crontal_cfgfile can not found!"
        result=$INSPECT_FAIL
    fi
    
    #检查上次运行时间
    local lastrun=`sed -n '$p' /var/space/var_space |awk '{print $1}'`
    echo "lastrun: $lastrun"
    local expectrun=`date "+%Y%m%d"`
    if [ "X$lastrun" != "X${expectrun}083000" ]; then
        result=$INSPECT_FAIL
    fi
    
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#==============================================================================
###内存占用检查
function cm_check_mem()
{
    local result=$INSPECT_OK
    
    INSPECT_START ${FUNCNAME}
    
    local memrate=`kstat -m unix -n system_pages |awk '$1=="availrmem"{availrmem=$2}$1=="physmem"{physmem=$2}END{rate=(1-availrmem/physmem)*100;print rate}'`
    memrate=`echo "$memrate" |awk -F'.' '{print $1}'`
    echo "memrate: $memrate%"
    if [ $memrate -ge 85 ]; then
        result=$INSPECT_FAIL
    fi
    
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#==============================================================================
###CPU占用检查
function cm_check_cpu()
{
    local result=$INSPECT_OK
    local sleep_time=2
    INSPECT_START ${FUNCNAME}
    
    local cpuinfo=`kstat -m cpu -n sys |grep cpu_ticks |awk 'BEGIN{ticks=0;ilds=0}$1=="cpu_ticks_idle"{ilds+=$2}{ticks+=$2}END{print ticks" "ilds}'`
    local info1=($cpuinfo)
    
    sleep $sleep_time
    
    cpuinfo=`kstat -m cpu -n sys |grep cpu_ticks |awk 'BEGIN{ticks=0;ilds=0}$1=="cpu_ticks_idle"{ilds+=$2}{ticks+=$2}END{print ticks" "ilds}'`
    local info2=($cpuinfo)
    local ticks=0
    ((ticks=${info2[0]}-${info1[0]}))
    local ilds=0
    ((ilds=${info2[1]}-${info1[1]}))
    local cpurate=0
    ((cpurate=100*(ticks-ilds)/ticks))

    echo "cpurate: $cpurate%"
    if [ $cpurate -ge 85 ]; then
        result=$INSPECT_FAIL
    fi
    
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#==============================================================================
###GUI 服务检查
function cm_check_gui()
{
    local result=$INSPECT_OK
    
    INSPECT_START ${FUNCNAME}
    
    local status=`svcs guiview |sed -n 2p |awk '{print $1}'`
    if [ "X$status" != "Xonline" ]; then
        echo "GUI state: $status"
        result=$INSPECT_FAIL
    fi
    
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#==============================================================================
###CLUMGT 服务检查
function cm_check_clumgt()
{
    local result=$INSPECT_OK
    
    INSPECT_START ${FUNCNAME}
    
    local status=`svcs clumgtd |sed -n 2p |awk '{print $1}'`
    if [ "X$status" != "Xonline" ]; then
        echo "clumgt state: $status"
        result=$INSPECT_FAIL
    fi
    
    local svcinfo='/var/svc/log/system-clumgtd:default.log'
    cnt=`grep 'dumped core' $svcinfo |wc -l |awk '{print $1}'`
    if [ $cnt -gt 0 ]; then
        local cntstart=1
        if [ $cnt -gt 5 ]; then
            ((cntstart=$cnt-5))
        fi
        grep 'dumped core' $svcinfo |sed -n "$cntstart,${cnt}p"
        result=$INSPECT_FAIL
    fi
    
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#==============================================================================
###CM服务检查
function cm_check_svc()
{
    local result=$INSPECT_OK
    local cnt=0
    INSPECT_START ${FUNCNAME}
    
    if [ ! -f /var/svc/manifest/system/ceres_cm.xml ]; then
        echo "no cm service"
        INSPECT_END ${FUNCNAME} $result
        return $result
    fi
    
    local status=`svcs ceres_cm |sed -n 2p |awk '{print $1}'`
    if [ "X$status" != "Xonline" ]; then
        echo "ceres_cm state: $status"
        result=$INSPECT_FAIL
    fi
    
    local cfginfo='/var/svc/log/system-ceres_cm:default.log'
    cnt=`grep 'dumped core' $cfginfo |wc -l |awk '{print $1}'`
    if [ $cnt -gt 0 ]; then
        local cntstart=1
        if [ $cnt -gt 5 ]; then
            ((cntstart=$cnt-5))
        fi
        grep 'dumped core' $cfginfo |sed -n "$cntstart,${cnt}p"
        result=$INSPECT_FAIL
    fi
    #CM集群配置是否一致
    local tmpfile='/tmp/cm_check_svc.tmp'
    rm -f $tmpfile
    cfginfo=('cat /var/cm/data/cm_cluster.ini' 'sqlite3 /var/cm/data/cm_node.db .dump' 'ceres_cmd master' 'ceres_cmd node')
    for((index=0;index<${#cfginfo[*]};index++))
    do
        cnt=`cm_multi_exec "${cfginfo[$index]}" |md5sum |awk 'BEGIN{a="";c=0}$1!=a{a=$1;c++}END{print c}'`
        if [ $cnt -gt 1 ]; then
            echo "${cfginfo[$index]} not the same"
            touch $tmpfile
        fi
    done
    
    #检查节点状态
    local nodecnt=`sqlite3 /var/cm/data/cm_node.db "select count(id) from record_t"`
    echo "node count: $nodecnt"
    if [ $nodecnt -gt 0 ]; then
        #检查每个节点的状态
        ceres_cmd node |sed 1d |while read line
        do
            #0     18488      dfa             192.168.36.56                  normal     1
            local nodeinfo=($line)
            if [ "X${nodeinfo[4]}" != "Xnormal" ]; then
                echo "$line"
                touch $tmpfile
            fi
        done
    else
        result=$INSPECT_FAIL
    fi
    
    if [ -f $tmpfile ]; then
        rm -f $tmpfile
        result=$INSPECT_FAIL
    fi
    
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#=============================================================================
#=============================================================================
inspect_main $*
exit $?
#=============================================================================
#=============================================================================
