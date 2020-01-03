#!/bin/bash

source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

FILE_NAME=`basename $0 |sed 's/.sh$//g'`
RDB_FILE='/var/cm/data/upgrade_rdb.txt'
STATE_FILE='/var/cm/data/upgrade.txt'
SERVER_FILE='/var/cm/script/cm_upgrade.py'
UPGRADE_DIR='/tmp/upgrade/'
MENU_LST='/root/boot/grub/menu.lst'

function cm_cnm_upgrade_init()
{
    if [ -f $RDB_FILE ]; then
        if [ `ps -ef |grep cm_upgrade.py|egrep -v grep|wc -l` -ne 0 ]; then
            #cm在升级进程重启前异常退出，不做任何处理
            return 0
        fi
        $SERVER_FILE
    fi
    
    return 0
}

function cm_cnm_upgrade_check()
{
    local gzfile=$1
    local rddir=`dirname $gzfile`
    local rdname=`basename $gzfile`
    local tarfile=`echo $rdname|sed 's/\.gz//g'`
    local unpack=`echo $tarfile|sed 's/\.tar//g'`
    local unpackdir=$UPGRADE_DIR$unpack
    
    if [ ! -d $UPGRADE_DIR ]; then
        mkdir -p $UPGRADE_DIR
    fi
    
    cp $gzfile $UPGRADE_DIR
    cd $UPGRADE_DIR
    echo $unpackdir
    gzip -d $rdname
    if [ $? -ne 0 ]; then
        return 1
    fi
    tar -xvf $tarfile
    if [ $? -ne 0 ]; then
        return 1
    fi
    cd ~
    rm -rf $UPGRADE_DIR*
    echo "rm -rf $UPGRADE_DIR*"
    cp $gzfile $UPGRADE_DIR
    return 0
}

function cm_cnm_upgrade_insert()
{
    local rdfile=$1
    local mode=$2
    local rdname=`basename $rdfile`
    
    if [ 'X'$rdfile = 'X' ] || [ 'X'$mode = 'X' ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] param error"
        return 1
    fi
    
    #检查clumgt.config配置
    if [ `cat /etc/clumgt.config|wc -l` -eq 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] config error"
        return 1
    fi
    #检查cm_multi_server服务是否正常运行
    if [ `cm_multi_exec hostname| grep fail | wc -l` -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] cm_multi_server fail"
        return 1
    fi
    #检查包的正确性
    cm_cnm_upgrade_check $rdfile
    if [ $? -ne 0 ]; then
        rm -rf $UPGRADE_DIR
        CM_LOG "[${FUNCNAME}:${LINENO}] check rd fail"
        return 1
    fi
    
    if [ -f $RDB_FILE ]; then
        cm_multi_exec "rm -rf $RDB_FILE"
    fi
    
    if [ -f $STATE_FILE ]; then
        cm_multi_exec "rm -rf $STATE_FILE"
    fi
    
    cm_multi_exec $SERVER_FILE #upgrade server start
    if [ $? -ne 0 ]; then
        cm_multi_exec "$SERVER_FILE exit"
        cm_multi_exec $SERVER_FILE
    fi
    cm_multi_exec "$SERVER_FILE $UPGRADE_DIR$rdname"
    $SERVER_FILE $mode
    
    return 0
}

function cm_cnm_upgrade_count()
{
    $SERVER_FILE get | grep df | wc -l
    
    return 0
}

function cm_cnm_upgrade_getbatch()
{
    $SERVER_FILE get |egrep -v server
    
    return 0
}

function cm_cnm_upgrade_process()
{
    local gz=$1
    local gzdir=`dirname $gz`
    local gzname=`basename $gz`
    local tarfile=`echo $gzname|sed 's/\.gz//g'`
    local unpack=`echo $tarfile|sed 's/\.tar//g'`
    local unpackdir=$UPGRADE_DIR$unpack
    
    if [ ! -f $gz ]; then
        return 1
    fi
    
    cd $UPGRADE_DIR
    gzip -d $gzname
    tar -xvf $tarfile
    cd -
    
    if [ `ls $unpackdir |grep upgrade|wc -l` -ne 0 ]; then
        cd $unpackdir
        rd=`ls $unpackdir |grep '\.rd'`
        rd_s=`echo $rd|awk -F'-' '{print $4}'`
        if [ `ls /root/pdyos/ |grep $rd_s|wc -l` -ne 0 ]; then
            rm -rf '/root/pdyos/$rd_s'
        fi
        './upgrade.sh' $rd
        cd -
    else
        cd $unpackdir
        tars=`ls $unpackdir |grep 'tar'`
        './patchinstall.sh' $tars
        cd -
    fi
    
    return 0
}

function cm_cnm_upgrade_version()
{
    local version=`cm_software_version`
    echo $version
}

function cm_cnm_upgrade_back()
{
    if [ `ls /root/pdyos/ | wc -l` -eq 1 ]; then
        return 0
    fi
    
    local bakfile=$MENU_LST'_bak'
    local start=`grep -n 'title ' $MENU_LST | awk -F':' '{print $1}'|head -1`
    local end=`grep -n '#\-' $MENU_LST | awk -F':' '{print $1}'|head -1`
    local version=`cat $MENU_LST|grep 'kernel '| awk -F'/' '{print $3}'|head -1`
    
    rm -rf "/root/pdyos/$version"
    
    cat $MENU_LST |sed "$start,$end d" > $bakfile
    mv $bakfile $MENU_LST
    
    return 0
}


$FILE_NAME'_'$*
exit $?
