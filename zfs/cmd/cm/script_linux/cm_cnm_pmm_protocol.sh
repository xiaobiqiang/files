#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

K_PRO_NAME=("smbsrv" "rfsproccnt_v2" "rfsproccnt_v3" "rfsproccnt_v4")
U_PRO_NAME=("cifs" "nfs_v2" "nfs_v3" "nfs_v4")


function cm_cnm_protocol_getbatch()
{
    for name in ${U_PRO_NAME[@]}
    do
        echo $name
    done
}


CM_PMM_PROTO_FILE=/tmp/protocol_data.$$

function cm_cnm_protocol_get_data()
{
    local name=$1
    local objs=($2)
    local obj_ret=0
    kstat -m nfs -c misc -n $name > $CM_PMM_PROTO_FILE
    for obj in ${objs[@]}
    do
       local num=$(cat $CM_PMM_PROTO_FILE|grep -w $obj|awk '{print $2}')
       if [ "$num"x == ""x ];then
           num=0
       fi
       ((obj_ret=$obj_ret+$num))
    done
    echo $obj_ret
    rm $CM_PMM_PROTO_FILE
}

main_Pobj="read write readdir getattr setattr lookup readlink create remove rename mkdir rmdir"
sub_Pobj="access compound getfh putfh putpubfh putrootfh nverify commit open openattr open_confirm renew restorefh savefh"

function cm_cnm_uname_kname()
{
    case $1 in
        cifs)
            echo smbsrv
        ;;
        nfs_v2)
            echo rfsproccnt_v2
        ;;
        nfs_v3)
            echo rfsproccnt_v3
        ;;
        nfs_v4)
            echo rfsproccnt_v4
        ;;
        *)
            echo null
        ;;
    esac
}

function cm_cnm_protocol_get()
{
    local protocol=$(cm_cnm_uname_kname $1)
    case $protocol in
        smbsrv)
            smbstat -t |sed '1,2d'|awk '{printf("%f\n"),$3}'
        ;;
        rfsproccnt_v2)
            cm_cnm_protocol_get_data rfsproccnt_v2 "$main_Pobj"
        ;;
        rfsproccnt_v3)
            cm_cnm_protocol_get_data rfsproccnt_v3 "$main_Pobj"
        ;;
        rfsproccnt_v4)
            cm_cnm_protocol_get_data rfsproccnt_v4 "$main_Pobj $sub_Pobj"
        ;;
        *)
            echo 0
        ;;
    esac

}

function cm_cnm_protocol_check()
{
    if [ "$1"x == "cifs"x ] || [ "$1"x == "nfs_v2"x ] || [ "$1"x == "nfs_v3"x ] || [ "$1"x == "nfs_v4"x ];then
        echo $CM_OK
    else
        echo $CM_FAIL
    fi
}

function cm_cnm_pmm_protocol_main()
{
    local cmd=$1
    local var=$2

    local iRet=$CM_OK
    case $cmd in
        getbatch)
            cm_cnm_protocol_getbatch
            iRet=$?
        ;;
        get)
            cm_cnm_protocol_get $var
            iRet=$?
        ;;
        check)
            cm_cnm_protocol_check $var
            iRet=$?
        ;;
    esac
    return $iRet
}

cm_cnm_pmm_protocol_main $1 $2
exit $?
