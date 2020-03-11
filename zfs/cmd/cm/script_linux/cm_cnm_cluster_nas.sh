#!/bin/bash  
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

function cm_cnm_cluster_nas_get()
{    
    local cluname=$1
    local sfver=`cm_systerm_version_get`
    
    if [ $sfver -eq $CM_SYS_VER_SOLARIS_NOMASTER ]; then
        zfs multiclus -v|sed 's/ //g'| awk -F'|' '$2=="Groupname"{aa=$3}$2=="Fsname"{bb=$3}$2=="Hostid"{cc=$3}$2=="Availsize"{dd=$3}$2=="Usedsize"{ee=$3}($2=="Status"&&aa=="'$cluname'"){print bb" "0" "$3" "dd" "ee" "cc}'
    else
        zfs multiclus -v|sed 's/ //g'| awk -F'|' '$2=="Groupname"{aa=$3}$2=="Fsname"{bb=$3}$2=="Nodetype"{cc=$3}$2=="Nodeid"{dd=$3}$2=="Availsize"{ee=$3}$2=="Usedsize"{ff=$3}($2=="Status"&&aa=="'$cluname'"){print bb" "cc" "$3" "ee" "ff" "dd}'
    fi
    return 0
}

function cm_cnm_cluster_nas_group()
{
    local sfver=`cm_systerm_version_get`
    
    if [ $sfver -eq $CM_SYS_VER_SOLARIS_NOMASTER ]; then
        zfs multiclus -v|sed 's/ //g'| awk -F'|' '$2=="Groupname"{aa=$3}$2=="Multiclusstatus"{bb=$3}$2=="Groupnumber"{cc=$3}$2=="Hostid"{print aa" "bb" "cc" "$3}'
    else
        zfs multiclus -v|sed 's/ //g'| awk -F'|' '$2=="Multiclusstatus"{aa=$3}$2=="Groupname"{bb=$3}$2=="Masternodeid"{cc=$3}$2=="Groupnumber"{print bb" "aa" "$3" "cc}'
    fi
    return 0
}

function cm_cnm_cluster_nas_count()
{
    local cluname=$1
    zfs multiclus -v |sed 's/ //g' |awk -F'|' 'BEGIN{num=0}$2=="Groupname"{a=$3}($2=="Groupnumber"&&a=="'$cluname'"){num=$3;break}END{print num}'
}

function cm_cnm_cluster_nas_setrole()
{
    local sfver=`cm_systerm_version_get`
    if [ $sfver -eq $CM_SYS_VER_SOLARIS_NOMASTER ]; then
        return 0
    fi
    local role=$1
    local cluname=$2
    local nas=$3
    zfs multiclus set $role $cluname $nas
    return $?
}

function cm_cnm_cluster_nas_delete()
{
    local cluname=$1
    local nas=$2
    local sfver=`cm_systerm_version_get`
    if [ $sfver -ne $CM_SYS_VER_SOLARIS_NOMASTER ]; then
        return 0
    fi
    zfs multiclus remove $cluname $nas
    return $?
}
cm_cnm_cluster_nas_$*
exit $?