#!/bin/bash

INSPECT_OK=0
INSPECT_FAIL=1
INSPECT_MOD_NAME=`basename $0 |sed 's/^inspect_//g' |sed 's/.sh$//g'`
INSPECT_PWD=$(dirname `readlink -f $0`)/
INSPECT_SH=`basename $0`
#==============================================================================
#巡检开始打印函数
function INSPECT_START()
{
    local func=$1
    local datestr=`date "+%Y%m%d%H%M%S"`
    printf "[%s]%-5s: %-20s\n" ${datestr} 'START' $func
}

#巡检结束打印函数
function INSPECT_END()
{
    local func=$1
    local result=$2
    if [ "X$result" == "X0" ]; then
        result="OK"
    else
        result="FAIL"
    fi
    local datestr=`date "+%Y%m%d%H%M%S"`
    printf "[%s]%-5s: %-20s RESULT: %5s\n\n" ${datestr} 'END' $func $result
}

#巡检模板检查函数
function inspect_demo()
{
    local result=$INSPECT_OK
    #开始
    INSPECT_START ${FUNCNAME}
    
    #执行相关检查，并设置巡检结果
    
    #结束
    INSPECT_END ${FUNCNAME} $result
    return $result
}

#获取巡检项列表
function inspect_list()
{
    local localfile=$0
    egrep '^###|^function '${INSPECT_MOD_NAME}'_' $localfile |sed 's/function //g' |sed 's/[(# )]//g' |xargs -n2 |awk '{print $2" "$1}'
    return $INSPECT_OK
}

#执行所有巡检
function inspect_all()
{
    local localfile=$0
    local result=$INSPECT_OK
    grep "^function ${INSPECT_MOD_NAME}_" $localfile | sed 's/function //g' |sed 's/()//g' |while read line
    do
        $line
    done
    return $result
}

#初始化检查
function inspect_init()
{
    local CM_SCRIPT_DIR='/var/cm/script/'
    local iRet=0
    if [ ! -d $CM_SCRIPT_DIR ]; then
        mkdir -p $CM_SCRIPT_DIR
        iRet=$?
        if [ $iRet -ne 0 ]; then
            echo "mkdir $CM_SCRIPT_DIR fail"
            return 1
        fi
    fi
    #检查cm_multi服务
    local server='cm_multi_server.py'
    local exec='cm_multi_exec'
    local curdir=`readlink -f $0` 
    curdir=`dirname $curdir`
    curdir=$curdir"/"
    if [ ! -f ${CM_SCRIPT_DIR}${server} ]; then
        cp ${curdir}${server} ${CM_SCRIPT_DIR}
    fi
    if [ ! -f ${CM_SCRIPT_DIR}${exec} ]; then
        cp "${curdir}${exec}.py" ${CM_SCRIPT_DIR}
    fi
    local isrun=`ps -ef|grep cm_multi_server |grep -v grep`
    if [ "X$isrun" == "X" ]; then
        #手动拉起
        if [ ! -d "/var/cm/log/" ]; then
            mkdir -p "/var/cm/log/"
        fi
        ${CM_SCRIPT_DIR}${server}
        sleep 1
        isrun=`ps -ef|grep cm_multi_server |grep -v grep`
        if [ "X$isrun" == "X" ]; then
            echo "start $server fail"
            return 1
        fi
    fi
    #检查软链接
    if [ ! -f /usr/sbin/${exec} ]; then
        ln -s "${CM_SCRIPT_DIR}${exec}.py" /usr/sbin/${exec}
    fi
    return 0
}

#恢复环境操作,必要的时候执行
function inspect_term()
{
    if [ ! -f /var/svc/manifest/system/cm_multi.xml ]; then
        #之前不存在cm_multi服务
        local serid=`ps -ef|grep cm_multi_server |grep -v grep |awk '{print $2}'`
        if [ "X$serid" != "X" ]; then
            echo "stop cm_multi[$serid]"
            kill -9 $serid
        fi
    fi
    return 0
}

#执行巡检主函数
function inspect_main()
{
    local action=$1
    if [ "X$action" == "X" ]; then
        inspect_list
        res=$?
    else
        $*
        res=$?
    fi
    return $res
}
