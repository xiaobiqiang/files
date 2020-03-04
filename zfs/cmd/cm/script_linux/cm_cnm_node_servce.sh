#!/bin/bash

source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

tmp_file="/tmp/txt$$"
tmp_file_bk="/tmp/txt_bk$$"

UNDEFINE=0
ONLINE=1
OFFLINE=2
PARTONLNE=3
MAINTENANCE=4
TRANSITION=5

servces=("nfs" "stmf" "ssh" "ftp" "smb" "guiview" "fmd" "iscsi")
sttic_sub_ser[0]="  /network/nfs/cbd
                    /network/nfs/status
                    /network/nfs/mapid
                    /network/nfs/nlockmgr
                    /network/nfs/client
                    /network/nfs/rquota
                    /network/nfs/server
"
sttic_sub_ser[1]="  /system/stmf"
sttic_sub_ser[2]="  /network/ssh"
sttic_sub_ser[3]="  /network/ftp"
sttic_sub_ser[4]="  /network/smb/client
                    /network/smb/server
                    /network/shares/group:smb
"
sttic_sub_ser[5]="  /system/guiview"
sttic_sub_ser[6]="  /system/fmd"
sttic_sub_ser[7]="  /network/iscsi/target"

function static_cm_cnm_servce_check()
{
    local serid=$1
    local servce=${servces[$serid]}
    
    local sub_sers=(${sttic_sub_ser[$serid]})
    
    local transition_lines=$(cat $tmp_file |grep -w ${servce}|grep '*'|wc -l)
    if [ $transition_lines -ne 0 ];then
        echo $TRANSITION
        return 
    fi
    
    local servce_lines=${#sub_sers[@]}
    #CM_LOG "[${FUNCNAME}:${LINENO}] $servce : ${sub_sers[@]}"
    local servce_online_lines=`cat $tmp_file |grep -w ${servce}|grep -w active|wc -l`
    #CM_LOG "$servce_lines $servce_online_lines"
    #if [ $servce_lines -eq 0 ];then
    #    echo $UNDEFINE
    if [ 1 -eq $servce_online_lines ];then
        echo $ONLINE
    else
        local ret=(`cat $tmp_file |grep -w ${servce}|awk '{print $3}'`)
        if [ "${ret[0]}"x == "failed"x ];then
            echo $OFFLINE
        elif [ "${ret[0]}"x == "maintenance"x ];then
            echo $MAINTENANCE
        else
            echo $UNDEFINE
        fi
    fi
}

function cm_cnm_servce_getbatch()
{
    
    local paranum=$#
    if [ $paranum -ne 0 ];then
        CM_LOG "[${FUNCNAME}:${LINENO}] err paranum [$paranum]"
        return $CM_PARAM_ERR
    fi

    local servce_status[0]=$UNDEFINE
    local index=0
    systemctl > $tmp_file
    for servce in ${servces[@]}
    do
        servce_status[$index]=`static_cm_cnm_servce_check $index`
        ((index=$index + 1))
    done
    rm $tmp_file
    echo ${servce_status[*]}
    return $CM_OK
}

function static_get_servce_instance()
{
    local servce=$1
    cat $tmp_file|grep $servce > $tmp_file_bk
    while read LINE
    do
        tmp=${LINE#*svc:}
        servce=${tmp%:default*}
        echo $servce
    done < $tmp_file_bk
    rm $tmp_file_bk
}

function static_cm_disable_servces()
{
    local sub_servces=($1)
    for sub_ser in ${sub_servces[@]}
    do
        svcadm disable $sub_ser 2>/dev/null
    done
}

function static_open_link_servce()
{
    local servce=$1
    local stat=$(svcs -l $servce|grep -w state|awk '{print $2}')
    if [ x"$stat" != x"online" ];then
        link_servces=$(svcs -l $servce|grep -w dependency|grep -w svc:|grep -v online|awk '{print $3}'|sed 's/svc:\///g')
        for ser in ${link_servces[@]}
        do
            svcadm enable $ser 2>/dev/null
        done
    fi
}

function static_cm_enable_servces()
{
    local sub_servces=($1)
    for sub_ser in ${sub_servces[@]}
    do
        svcadm enable $sub_ser 2>/dev/null
        static_open_link_servce $sub_ser
    done
}

function cm_cnm_servce_updata()
{
    local paranum=$#
    if [ $paranum -ne 2 ];then
        CM_LOG "[${FUNCNAME}:${LINENO}] err paranum [$paranum]"
        return $CM_PARAM_ERR
    fi
    local serid=$1
    local status=$2
    local sub_servces=${sttic_sub_ser[$serid]}
    
    svcs|grep ${servces[$serid]} > $tmp_file
    local start_status=$(static_cm_cnm_servce_check $serid)
    if [ $start_status -eq $status ];then
        return $CM_OK    
    elif [ $start_status -eq $TRANSITION ];then
        return $CM_FAIL
    fi
    
    CM_LOG "[${FUNCNAME}:${LINENO}]sub_servces:$sub_servces"
    if [ $status -eq $UNDEFINE ];then
        static_cm_disable_servces "$sub_servces"
    elif [ $status -eq $ONLINE ];then
        static_cm_disable_servces "$sub_servces"
        static_cm_enable_servces "$sub_servces"
    else
        return $CM_OK
    fi
    
    svcs|grep ${servces[$serid]} > $tmp_file
    local finl_status=`static_cm_cnm_servce_check $serid`
    if [ $finl_status -ne $status ]&&[ $finl_status -ne $TRANSITION ];then
        CM_LOG "[${FUNCNAME}:${LINENO}] ser[$serid]:${servces[$serid]} turn to $status currten is  $finl_status FAIL"
        return $CM_FAIL
    fi
    rm $tmp_file
    return $CM_OK
}

function cm_cnm_servce_main()
{
    local fun=$1
    local cmd=$2
    case $fun in
        
        getbatch)
            cm_cnm_servce_getbatch
            iRet=$?
        ;;
        update)
            cm_cnm_servce_updata $cmd
            iRet=$?
        ;;
    esac
    return $iRet
}

cm_cnm_servce_main $1 "$2 $3"
exit $?
