#!/bin/bash

source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'


UNDEFINE=0
ONLINE=1
OFFLINE=0
PARTONLNE=3
MAINTENANCE=4
TRANSITION=5

os_type=`cm_systerm_version_get`

servces=("nfs" "stmf" "ssh" "ftp" "smb" "guiview" "fmd" "iscsi")

if [ $os_type -eq $CM_OS_TYPE_DEEPIN ];then
    sttic_sub_ser[0]="  nfs-mountd.service"
else
    sttic_sub_ser[0]="  nfs.service"
fi
    
sttic_sub_ser[1]="  stmf-svc.service"
sttic_sub_ser[2]="  sshd.service"
sttic_sub_ser[3]="  vsftpd.service"

if [ $os_type -eq $CM_OS_TYPE_DEEPIN ];then
    sttic_sub_ser[4]="  smbd.service"
else
    sttic_sub_ser[4]="  smb.service"
fi

sttic_sub_ser[5]="  guiview.service"
sttic_sub_ser[6]="  fmd.service"

if [ $os_type -eq $CM_OS_TYPE_DEEPIN ];then
    sttic_sub_ser[7]="  iscsitsvc.service"
else
    sttic_sub_ser[7]="  iscsi.service"
fi


function static_cm_cnm_servce_check()
{
    local serid=$1
    if [ $serid -eq 5 ];then
        echo $ONLINE
        return $CM_OK
    fi
    #local servce=${servces[$serid]}
    
    local sub_sers=(${sttic_sub_ser[$serid]})
    
    local transition_lines=`systemctl status $sub_sers |grep Active|awk '{print $2}'`
    if [ "X$transition_lines" == "Xactive" ];then
        echo $ONLINE
    elif [ "X$transition_lines" == "Xinactive" ];then
        echo $OFFLINE
    else
        echo $UNDEFINE
    fi
    return $?
    
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
    
    for servce in ${servces[@]}
    do
        servce_status[$index]=`static_cm_cnm_servce_check $index`
        ((index=$index + 1))
    done

    echo ${servce_status[*]}
    return $CM_OK
}

function static_cm_disable_servces()
{
    local sub_servces=($1)
    for sub_ser in ${sub_servces[@]}
    do
        systemctl stop $sub_ser
    done
    return $?
}

function static_cm_enable_servces()
{
    local sub_servces=($1)
    for sub_ser in ${sub_servces[@]}
    do
        systemctl start $sub_ser
    done
    return $?  
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
    
    local start_status=$(static_cm_cnm_servce_check $serid)
    if [ $start_status -eq $status ];then
        return $CM_OK    
    fi
    
    CM_LOG "[${FUNCNAME}:${LINENO}]sub_servces:$sub_servces"
    if [ $status -eq $OFFLINE ];then
        static_cm_disable_servces "$sub_servces"
    elif [ $status -eq $ONLINE ];then
        static_cm_enable_servces "$sub_servces"
    else
        return $CM_OK
    fi
    
    local finl_status=`static_cm_cnm_servce_check $serid`
    if [ $finl_status -ne $status ];then
        CM_LOG "[${FUNCNAME}:${LINENO}] ser[$serid]:${servces[$serid]} turn to $status currten is  $finl_status FAIL"
        return $CM_FAIL
    fi
    return $CM_OK
}

function cm_cnm_server_iscsi_check()
{
    local sname=${sttic_sub_ser[7]}
    local status=`systemctl status $sname|grep Active|awk '{print $2}'`
    if [ "X$status" == "Xactive" ]; then
        return $CM_OK
    fi
    
    CM_EXEC_CMD "systemctl stop $sname"
    CM_EXEC_CMD "systemctl start $sname"
    return $?
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
        iscsi_check)
            cm_cnm_server_iscsi_check
            iRet=$?
        ;;
        *)
        ;;
    esac
    return $iRet
}

cm_cnm_servce_main $1 "$2 $3"
exit $?
