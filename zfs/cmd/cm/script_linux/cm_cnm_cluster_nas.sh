#!/bin/bash  

function cm_cnm_cluster_nas_get()
{    
    local cluname=$1
    zfs multiclus -v|sed 's/ //g'| awk -F'|' '$2=="Groupname"{aa=$3}$2=="Fsname"{bb=$3}$2=="Nodetype"{cc=$3}$2=="Nodeid"{dd=$3}$2=="Availsize"{ee=$3}$2=="Usedsize"{ff=$3}($2=="Status"&&aa=="'$cluname'"){print bb" "cc" "$3" "ee" "ff" "dd}'
}

function cm_cnm_cluster_nas_group()
{
    zfs multiclus -v|sed 's/ //g'| awk -F'|' '$2=="Multiclusstatus"{aa=$3}$2=="Groupname"{bb=$3}$2=="Masternodeid"{cc=$3}$2=="Groupnumber"{print bb" "aa" "$3" "cc}'
}

function cm_cnm_cluster_nas_count()
{
    local cluname=$1
    zfs multiclus -v |sed 's/ //g' |awk -F'|' 'BEGIN{num=0}$2=="Groupname"{a=$3}($2=="Groupnumber"&&a=="'$cluname'"){num=$3;break}END{print num}'
}

function cm_cnm_cluster_nas_setrole()
{
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
    echo "this version do not support detele"
    return $?
}
cm_cnm_cluster_nas_$*


exit $?