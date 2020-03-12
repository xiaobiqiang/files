#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

VERSION=`cm_systerm_version_get`

function cm_cnm_dirquota_getbatch()
{
    local names=$(zfs list|awk '{print $5}'|sed -n '/\/.*\//'p|awk -F'/' '{print $2 "/" $3}')
    for name in ${names[@]}
    do
        if [ $VERSION -eq $CM_SYS_VER_SOLARIS_V7R16 ];then
                zfs dirspace $name 2>/dev/null|sed 's/%//g'
        else
                zfs dirspace $name 2>/dev/null|sed 's/%//g'
        fi      
    done
    return $CM_OK
}

function cm_cnm_dirquota_get()
{
    local name=$1
    local path=$2
    if [ $path == "all" ];then
        if [ $VERSION -eq $CM_SYS_VER_SOLARIS_V7R16 ];then
                zfs dirspace $name 2>/dev/null|sed 's/%//g'
        else
                zfs dirspace $name 2>/dev/null|sed 's/%//g'
        fi    
       return $?
    fi
    if [ $VERSION -eq $CM_SYS_VER_SOLARIS_V7R16 ];then
            zfs dirspace $name 2>/dev/null|sed 's/%//g'|grep -w $path
    else
            zfs dirspace $name 2>/dev/null|sed 's/%//g'|grep -w $path
    fi
    return $?
}

function cm_cnm_dirquota_add()
{
    local nas=$1
    local dir=$2
    local quota=$3

    zfs dirquota@$dir=$3 $nas
    return $?
}

function cm_cnm_pmm_dirquota_main()
{
    local cmd=$1
    local var=$2

    local iRet=$CM_OK
    case $cmd in
        getbatch)
            cm_cnm_dirquota_getbatch
            iRet=$?
        ;;
        get)
            cm_cnm_dirquota_get $var
            iRet=$?
        ;;
        add)
            cm_cnm_dirquota_add $var
            iRet=$?
        ;;
    esac
    return $iRet
}

cm_cnm_pmm_dirquota_main $1 "$2 $3 $4"
exit $?
