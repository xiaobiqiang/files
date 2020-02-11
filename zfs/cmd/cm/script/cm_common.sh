#!/bin/bash

CM_LOG_DIR="/var/cm/log/"
CM_LOG_FILE=$0
CM_LOG_FILE=${CM_LOG_FILE##*/}".log"
CM_LOG_ENABLE=1

#日志打印级别定义
CM_LOG_LEVEL_OFF=0
CM_LOG_LEVEL_ERR=1
CM_LOG_LEVEL_WARNING=2
CM_LOG_LEVEL_INFO=3
CM_LOG_LEVEL_DEBUG=4
CM_LOG_LEVEL=$CM_LOG_LEVEL_ERR

CM_SYS_VER_DEFAULT=0
CM_SYS_VER_SOLARIS_V7R16=1

CM_OS_TYPE_SOLARIS=0
CM_OS_TYPE_ILLUMOS=1

CM_NODE_DB_FILE="/var/cm/data/cm_node.db"

#==================================================================================
#                               日志打印
# 输入参数:
#       *
# 回显数据:
#       null
# 返 回 值:
#       null
# 用法: 
#   1. 在脚本中 source '/var/cm/script/cm_common.sh'
#   2. CM_LOG_ENABLE=1 开启打印日志，0:关闭
#   3. 调用方法 CM_LOG "[${FUNCNAME}:${LINENO}]xxxx"
#==================================================================================
function CM_LOG()
{
    if [ $CM_LOG_ENABLE -eq 1 ]; then
        local datestr=`date "+%Y%m%d%H%M%S"`
        echo "[${datestr}]$@" >>${CM_LOG_DIR}${CM_LOG_FILE}
    fi
}

function CM_LOG_ERR()
{
    if [ $CM_LOG_LEVEL -ge $CM_LOG_LEVEL_ERR ]; then
        local datestr=`date "+%Y%m%d%H%M%S"`
        echo "[${datestr}]Error:$@" >>${CM_LOG_DIR}${CM_LOG_FILE}
    fi
}

function CM_LOG_WARNING()
{
    if [ $CM_LOG_LEVEL -ge $CM_LOG_LEVEL_WARNING ]; then
        local datestr=`date "+%Y%m%d%H%M%S"`
        echo "[${datestr}]Warning:$@" >>${CM_LOG_DIR}${CM_LOG_FILE}
    fi
}

function CM_LOG_INFO()
{
    if [ $CM_LOG_LEVEL -ge $CM_LOG_LEVEL_INFO ]; then
        local datestr=`date "+%Y%m%d%H%M%S"`
        echo "[${datestr}]Info:$@" >>${CM_LOG_DIR}${CM_LOG_FILE}
    fi
}

function CM_LOG_DEBUG()
{
    if [ $CM_LOG_LEVEL -ge $CM_LOG_LEVEL_DEBUG ]; then
        local datestr=`date "+%Y%m%d%H%M%S"`
        echo "[${datestr}]Debug:$@" >>${CM_LOG_DIR}${CM_LOG_FILE}
    fi
}

function CM_EXEC_CMD()
{
    echo "$*" >> ${CM_LOG_DIR}${CM_LOG_FILE}
    $* >> ${CM_LOG_DIR}${CM_LOG_FILE} 2>&1
    return $?
}

function cm_systerm_version_get()
{
    local issolaris=`uname -a |grep -w Prodigy |wc -l`
    if [ $issolaris -ne 0 ]; then
        #Solaris V1110crypt.20.v7.r16.p11
        local versions=`cat /lib/release |sed -n 1p |awk '{print $5}' |awk -F'.' '{print $3"."$4}'`
        if [ "X$versions" == "Xv7.r16" ]; then
            echo $CM_SYS_VER_SOLARIS_V7R16
            return 0
        fi
    fi
    echo $CM_SYS_VER_DEFAULT
    return 0
}

function cm_os_type_get()
{
    local osname=`uname`
    case ${osname} in
        Prodigy)
            echo ${CM_OS_TYPE_SOLARIS}
        ;;
        SunOS)
            echo ${CM_OS_TYPE_ILLUMOS}
        ;;
        *)
            echo ${CM_OS_TYPE_SOLARIS}
        ;;
    esac
}

function cm_get_localid()
{
    local interid=`hostid`
    interid=`printf %d "0x$interid"`
    echo $interid
    return 0
}

function cm_get_localenid()
{
    local interid=`cm_get_localid`
    ((interid=(($interid+1)>>1)*1000))
    echo $interid
    return 0
}

function cm_check_isipaddr()
{
    local ip=$1
    if [ "X$ip" == "X" ]; then
        return 1
    fi
    local param="^(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[1-9])\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[1-9])\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[1-9])\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[1-9])$"
    local cnt=`echo "$ip" |egrep $param |wc -l`
    if [ $cnt -gt 0 ]; then
        return 0
    else
        return 1
    fi
}

function cm_software_version()
{
    local issolaris=`uname -a |grep -w Prodigy |wc -l`
    if [ $issolaris -ne 0 ]; then
        sed -n 1p /lib/release |awk -F'.' '{print $3$4$5}' |sed 's/PRO//g'
    else
        sed -n 1p /etc/release |awk '{print $2$3}'
    fi
}

function cm_get_localmanageport()
{
    local cfgfile='/var/cm/data/node_config.ini'
    if [ ! -f ${cfgfile} ]; then
        echo "igb0"
        return 0
    fi
    local mport=`grep '^mport' $cfgfile |sed 's/ //g' |awk -F'=' '{print $2}'`
    if [ "X$mport" != "X" ]; then
        echo $mport
        return 0
    fi
    echo "igb0"
    return 0
}

function cm_get_localmanageip()
{
    local mport=`cm_get_localmanageport`
    local cfgfile='/etc/hostname.'$mport
    
    if [ ! -f ${cfgfile} ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}]$cfgfile not exist"
        echo "0.0.0.0"
        return 0
    fi
    sed -n 1p ${cfgfile}
    return 0
}

function cm_check_isnum()
{
    local num=$1
    if [ "X$num" == "X" ]; then
        return 1
    fi
    local param='^[1-9][0-9]*$'
    local cnt=`echo "$num" |egrep $param |wc -l`
    if [ $cnt -gt 0 ]; then
        return 0
    else
        return 1
    fi
}
