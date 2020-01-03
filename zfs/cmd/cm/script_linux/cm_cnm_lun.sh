#!/bin/bash

source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

# CM_LOG "[${FUNCNAME}:${LINENO}]xxxx"

filename=$(cd `dirname $0`;pwd)
thfile=$0
thfile=${thfile##*/}
CM_CNN_LUN_SCRIPT_FILE=$filename"/"$thfile

# CM_LOG "[${FUNCNAME}:${LINENO}]xxxx"
CM_CNM_LUN_MIRROR_ERR_STATE=111
CM_CNM_LUN_BACKUP_ERR_OCC=112
CM_ETC_HOSTS='/etc/inet/hosts'
CM_ETC_HOSTS_TMP='/tmp/cm_cnm_lun_inet_hosts'

#双活条件检查
function cm_cnm_lun_mirror_local_check()
{
    local lun=$1
    local stmfid=`stmfadm list-all-luns 2>/dev/null |grep -w $lun|awk '{print $1}'`
    # 1.状态必须是Active->Standby
    if [ "X$stmfid" == "X" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] get $lun id fail"
        return $CM_CNM_LUN_MIRROR_ERR_STATE
    fi
    local state=`stmfadm list-lu -v $stmfid 2>/dev/null |awk '$1=="Access"{print $4}'`
    if [ "X$state" != "XActive->Standby" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] $lun state=$state"
        return $CM_CNM_LUN_MIRROR_ERR_STATE
    fi
    return $CM_OK
}

function cm_cnm_lun_mirror_check()
{
    #对于本节点lun
    local lun=$1
    local dest_ip=$2
    local iRet=$CM_OK
    cm_cnm_lun_mirror_local_check $lun
    iRet=$?
    if [ $iRet -ne $CM_OK ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] $lun local iRet=$iRet"
        return $iRet
    fi
    #检查远端
    ceres_exec $dest_ip "$CM_CNN_LUN_SCRIPT_FILE mirror_check '$lun'"
    iRet=$?
    if [ $iRet -ne $CM_OK ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] $lun $dest_ip iRet=$iRet"
        return $iRet
    fi
    return $CM_OK
}

#备份条件检查
function cm_cnm_lun_backup_local_check()
{
    local lun=$1
    local stmfid=`stmfadm list-all-luns 2>/dev/null |grep -w $lun|awk '{print $1}'`
    # 1.状态必须是Active
    if [ "X$stmfid" == "X" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] get $lun id fail"
        return $CM_CNM_LUN_MIRROR_ERR_STATE
    fi
    local state=`stmfadm list-lu -v $stmfid 2>/dev/null |awk '$1=="Access"{print $4}'`
    if [ "X$state" != "XActive" ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] $lun state=$state"
        return $CM_CNM_LUN_MIRROR_ERR_STATE
    fi
    return $CM_OK
}

function cm_cnm_lun_backup_check()
{
    #对于本节点lun
    local lun=$1
    local dest_ip=$2
    local rlun=$3
    local iRet=$CM_OK
    cm_cnm_lun_backup_local_check $lun
    iRet=$?
    if [ $iRet -ne $CM_OK ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] $lun local iRet=$iRet"
        return $iRet
    fi
    #检查远端
    # 检查备份卷是否已经被占用
    local dol0='$0'
    local dol1='$1'
    local isocc_vol=$(timeout 3 ceres_exec $dest_ip "sndradm -P | awk '{a[NR]=$dol0;if($dol0~/set: 2active/ && a[NR-1]~/<-/){a[NR]=a[NR-1]}{print $dol1}}'| grep -w $rlun")
    if [ ! -z $isocc_vol ];then
        CM_LOG "[${FUNCNAME}:${LINENO}] $rlun is busy"
        return $CM_CNM_LUN_BACKUP_ERR_OCC
    fi
    ceres_exec $dest_ip "$CM_CNN_LUN_SCRIPT_FILE backup_check '$rlun'"
    iRet=$?
    if [ $iRet -ne $CM_OK ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] $lun $dest_ip iRet=$iRet"
        return $iRet
    fi
    return $CM_OK
}

# 创建位图卷
# param: $path $bmpath
function cm_cnm_san_seki_create_bitmap_vol 
{   
    # 获取位图卷大小
    local vsize=`zfs list -H -o volsize $1`
    [ -z $vsize ]&&return $CM_FAIL
    local nsize=${vsize:0:${#vsize}-1}
    local vunit=${vsize:${#vsize}-1:1}
    local bsize=
    case $vunit in
        T):
            bsize='5G'  # 大于以T为单位的卷位图卷最大值
            ;;
        G):
            bsize='5M'  # 大于以G为单位的卷位图卷最大值
            ;;
        M):
            bsize='5K'  # 大于以M为单位的卷位图卷最大值
            ;;
        *):
            return $CM_FAIL
            ;;
    esac

    # 创建位图卷
    zfs create -V $bsize $2
    
    # # 删除位图卷的LU
    local my_bm_vol_lu=$(stmfadm list-all-luns | grep -w $2 | awk '{print $1}')
    [ -z $my_bm_vol_lu ]&&return $CM_FAIL
    stmfadm delete-lu $my_bm_vol_lu
    return $?
}

# 为数据卷添加视图
# param: $vol_lu
function cm_cnm_san_seki_vol_add_view
{
    stmfadm add-view $1 2>/dev/null
    return $CM_OK
}

# 给池设置网口、ip和掩码
# param: $path $nic $ip "${netmask}"
function cm_cnm_san_seki_set_pool_ip
{
    local netmask=255.255.255.0
    [ ! -z $4 ]&&netmask=$4
    local path=$1
    local poolname=${path%%/*}
    local rdc_ip=$(zfs get -H rdc:ip_owner $poolname | awk '{print $3}')
    if [ -z $3 ]; then 
        #之前和现在都没设置rdc:ip_owner
        [ $rdc_ip == - ]&&return $CM_ERR_CNM_SAN_SEKI_BIND_POOL_IP
        return $CM_OK
    fi
    zfs set rdc:ip_owner=$2,$3,${netmask} $poolname
    ifconfig $2 mtu 1500
    return $CM_OK
}

# /etc/hosts追加ip
# $ip_1 $rdc_1 $ip_2 $rdc_2 $host_1 $host_2
function cm_cnm_san_seki_app_etc_hosts 
{
    local errappfl=0
    local err_ip= 
    local host_1=$5
    local host_2=$6
    local lname=$(cat /etc/nodename)
    local lsbbid=$(sqlite3 /var/cm/data/cm_node.db "SELECT sbbid FROM record_t WHERE name='${lname}'")
    local rinfo=$(sqlite3 /var/cm/data/cm_node.db "SELECT ipaddr,name FROM record_t WHERE sbbid=${lsbbid} AND name!='${lname}'")
    if [ ! -z $rinfo ]; then 
        local rname=${rinfo##*|}
        local h1_name=${host_1%%_*}
        local h2_name=${host_2%%_*}
        #考虑到同一sbbid上的双控间的LUN备份
        #而且有可能是互相漂移过来的，所以直接比较两个
        [ "$rname" != "$h2_name" -a "$rname" != "$h1_name" ]&&errappfl=1
        err_ip=${rinfo%%|*}
    fi
    #同时向故障切换节点追加记录
    if [ ! -z $1 ]; then
        echo $1 $5 >>${CM_ETC_HOSTS}
        [ $errappfl -eq 1 ]&&timeout 1 ceres_exec $err_ip "echo $1 $5 >>${CM_ETC_HOSTS}"
    fi
    if [ ! -z $3 ]; then 
        echo $3 $6 >>${CM_ETC_HOSTS}
        [ $errappfl -eq 1 ]&&timeout 1 ceres_exec $err_ip "echo $3 $6 >>${CM_ETC_HOSTS}"
    else    
        local cnt=$(cat ${CM_ETC_HOSTS} | grep -w $4 | wc -l)
        if [ $cnt -eq 0 ]; then 
            echo $4 $6 >>${CM_ETC_HOSTS}
            [ $errappfl -eq 1 ]&&timeout 1 ceres_exec $err_ip "echo $4 $6 >>${CM_ETC_HOSTS}"
        fi
    fi  
    return $CM_OK
}

# 开启avs服务
# param: none
function cm_cnm_san_seki_start_avs
{
    local cnt=`dscfgadm -i | awk 'NR>=2{if($2=="disabled"){print}}' | wc -l`
    if [ $cnt -ne 0 ]; then     # 没有开启avs服务
/usr/bin/expect <<EOF
set timeout 2
set resp "y"
spawn dscfgadm
expect "?]"
send "y\r"
interact
expect eof
EOF
    fi
    return $?
}

# $path $bmpath $vol_lu "$nic" "$ip_1" "$loc_rdc" "$ip_2" "$rem_rdc" $name $rname "$netmask"
function cm_cnm_san_seki_pre_main
{
    # 创两个数据卷的位图卷
    cm_cnm_san_seki_create_bitmap_vol $1 $2
    if [ $? -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] create bitmap volume fail"
        return $CM_ERR_CNM_SAN_SEKI_CRT_BM_VOL
    fi
    
    # 为两个数据卷添加视图
    #cm_cnm_san_seki_vol_add_view "$3"
    #if [ $? -ne 0 ]; then
    #    CM_LOG "[${FUNCNAME}:${LINENO}] add view fail"
    #    return $CM_ERR_CNM_SAN_SEKI_ADD_POOL_VIEW
    #fi
    
    # 给池绑定网口和IP：path nic ip
    cm_cnm_san_seki_set_pool_ip $1 "$4" "$5" "${11}"
    if [ $? -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] set pool ip fail"
        return $CM_ERR_CNM_SAN_SEKI_BIND_POOL_IP
    fi
    
    # 追加/etc/hosts
    cm_cnm_san_seki_app_etc_hosts "$5" "$6" "$7" "$8" "$9" "${10}"
    if [ $? -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] append ip and hostname fail"
        return $CM_ERR_CNM_SAN_SEKI_APP_HOSTS
    fi
    
    # 检查avs服务
    cm_cnm_san_seki_start_avs
    if [ $? -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] check avs service fail"
        return $CM_ERR_CNM_SAN_SEKI_CONF_AVS
    fi
    return $CM_OK
}

# 配置双活avs服务
# param: $path $bmpath $host_1 $host_2 $flag(local,remote) "${issync}" (SYNC,ASYNC)
# cnt: 7
function cm_cnm_san_seki_only_avs_conf
{
    local opt="-Ej"
    local sync="sync"
    [ "$6" == "ON" ]&&opt="-ej"
    [ "$7" == "ASYNC" ]&&sync="async"
    if [ "$5" == "remote" ]; then
    local iRet=$CM_OK
    # 配置avs服务
/usr/bin/expect <<EOF
set timeout 5
spawn sndradm $opt $4 /dev/zvol/rdsk/$1 /dev/zvol/rdsk/$2 $3 /dev/zvol/rdsk/$1 /dev/zvol/rdsk/$2 ip $sync
expect "]:"
send "y\r"
interact
expect eof
catch wait ret
exit [lindex \$ret 3]
EOF
    iRet=$?
    else
/usr/bin/expect <<EOF
set timeout 5
spawn sndradm $opt $3 /dev/zvol/rdsk/$1 /dev/zvol/rdsk/$2 $4 /dev/zvol/rdsk/$1 /dev/zvol/rdsk/$2 ip $sync
expect "]:"
send "y\r"
interact
expect eof
catch wait ret
exit [lindex \$ret 3]
EOF
    iRet=$?
    fi
    return $iRet
}

# param: $path $bmpath $vol_lu "$nic" "$ip_1""$loc_rdc" "$ip_2" "$rem_rdc" $name $rname "$issync" "netmask" (SYNC,ASYNC)
# cnt: 13
function cm_cnm_san_seki_only_main_each
{   
    cm_cnm_san_seki_pre_main $1 $2 $3 "$4" "$5" "$6" "$7" "$8" "$9" "${10}" "${12}"
    local rs=$?
    if [ $rs -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] prepare fail"
        return $rs
    fi
    
    # 配置和启动AVS服务
    cm_cnm_san_seki_only_avs_conf $1 $2 "$9" "${10}" 'remote' "${11}" "${13}"
    if [ $? -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] config avs service"
        return $CM_ERR_CNM_SAN_SEKI_CONF_AVS
    fi
    return $CM_OK
}

# nid_1 nid_2 path (nic ip_1 ip_2 issync netmask_1 netmask_2)可能为"" (SYNC,ASYNC)
# cnt: 10
function cm_cnm_san_seki_only_main
{
    # 检查节点是否在cluster san中
    local nodes=(`sqlite3 /var/cm/data/cm_node.db "SELECT ipaddr,sbbid FROM record_t WHERE id in ($1,$2)"`)
    if [ ${#nodes[@]} -lt 2 ]; then     # 缺记录
        CM_LOG "[${FUNCNAME}:${LINENO}] less record"
        return $CM_ERR_CNM_SAN_SEKI_NO_RECORD
    fi
    
    # 两个节点的sbbid不能相同
    local infos=(`echo ${nodes[@]} | sed 's/|/ /g'`)    # ip1 sbbid1 ip2 sbbid2
    local sbbids=(${infos[1]} ${infos[3]})
    
    local ip=`cm_get_localmanageip`
    local ips=(${infos[0]} ${infos[2]})
    [ $ip == ${ips[1]} ]&&ips=(${infos[2]} ${infos[0]})
    if [ ${sbbids[0]} -eq ${sbbids[1]} ]; then  # sbbid相等
        CM_LOG "[${FUNCNAME}:${LINENO}] the same sbbid"
        return $CM_ERR_CNM_SAN_SEKI_SAME_SBBID
    fi
    
    # 检查两个卷的LU是否相同
    local path=$3   #不检查两个节点都存在这个路径，上层确定
    local my_vol_lu=$(stmfadm list-all-luns | grep -w $path | awk '{print $1}')
    local dsk_path=$(timeout 2 ceres_exec ${ips[1]} "stmfadm list-lu -v ${my_vol_lu} | awk '/Data/'| cut -d ':' -f 2")
    if [ /dev/zvol/rdsk/${path} != ${dsk_path} ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] LU name is different"
        return $CM_ERR_CNM_SAN_SEKI_DIFF_LU_NAME
    fi
    
    cm_cnm_lun_mirror_check $path ${ips[1]}
    local iRet=$?
    if [ $iRet -ne $CM_OK ];then
        return $iRet
    fi
    
    local timestamp=`date "+%Y%m%d%H%M%S"`  
    
    # 数据卷LU NAME
    local vol_lu=$my_vol_lu 
    # 位图卷路径
    local bmpath=${path%/*}/bitmap_vol_${timestamp}_bit0
    # rdc:ip hostname
    local name=$(hostname)_$timestamp
    local rname=`timeout 1 ceres_exec ${ips[1]} "hostname"`_$timestamp  
    local loc_rdc=
    local rem_rdc=
    if [ -z $5 ]; then
        loc_rdc=$(zfs get -H rdc:ip_owner $path | awk '{print $3}' | cut -d ',' -f 2)
        name=$(cat ${CM_ETC_HOSTS} | grep -w $loc_rdc | awk '{print $2}'|head -1)
    fi
    if [ -z $6 ]; then
        rem_rdc=$(timeout 2 ceres_exec ${ips[1]} "zfs get -H rdc:ip_owner $path | awk '{print \$3}' | cut -d ',' -f 2")
        rname=$(timeout 1 ceres_exec ${ips[1]} "cat ${CM_ETC_HOSTS} | grep -w $rem_rdc | awk '{print \$2}'|head -1")
    fi
    [ -z "${name}" -o -z "${rname}" ]&&return $CM_FAIL
    cm_cnm_san_seki_pre_main $path $bmpath $vol_lu "$4" "$5" "$loc_rdc" "$6" "$rem_rdc" "$name" "$rname" "$8" #8:netmask_1
    local rs=$?
    if [ $rs -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] prepare fail"
        return $rs
    fi
    
    [ -z $name -o -z $rname ]&&return $CM_FAIL
    #$path $bmpath $vol_lu "$nic" "$ip_1""$loc_rdc" "$ip_2" "$rem_rdc" $name $rname $issync $netmask_1 (SYNC,ASYNC)
    timeout 15 ceres_exec ${ips[1]} "/var/cm/script/cm_cnm_lun.sh seki_main_each $path $bmpath '${vol_lu}' '$4' '$6' '${rem_rdc}' '$5' '${loc_rdc}' $rname $name '$7' '$9' '${10}'"
    rs=$?
    if [ $rs -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] remote config fail"
        return $rs
    fi
    
    # 配置和启动AVS服务
    cm_cnm_san_seki_only_avs_conf $path $bmpath $name $rname 'local' "$7" "${10}"
    if [ $? -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] config avs service"
        return $CM_ERR_CNM_SAN_SEKI_CONF_AVS
    fi
    return $CM_OK
}

# param: none
# echo:  count
function cm_cnm_san_seki_only_count
{
    local cnt=$(sndradm -P | awk '{a[NR]=$0;if($0~/set: 2active/ && a[NR-1]~/->/){print}}' | wc -l)
    echo $cnt
    return $?
}

# param:offset total linearr flag(ASYNC, SYNC)
function cm_cnm_san_seki_only_getbatch_comm
{
    local flag=$4
    local line=($3)     #要加括号
    local tmp=
    ((tmp=2*$1))
    local total=
    ((total=2*$2))
    local end=
    ((end=$tmp+$total))
    local path=
    local nic=
    local snid=
    local rdc_ip_master=
    local rdc_ip_slave=
    local netmask_master=
    local netmask_slave=
    while [[ $tmp -lt $end && ! -z ${line[$tmp]} ]]; do
        path=$(echo ${line[$tmp]##*:} | sed 's/\/dev\/zvol\/rdsk\///')
        local rdc_info_1=$(zfs get -H rdc:ip_owner $path | awk '{print $3}')
        rdc_ip_master=$(echo $rdc_info_1 | cut -d ',' -f 2)
        netmask_master=${rdc_info_1##*,}
        nic=${rdc_info_1%%,*}
        local rdc_info_2=${line[$tmp+1]}
        local rdc_host=${rdc_info_2%%:*}
        local rem_host=${rdc_host%%_*}
        rdc_ip_slave=$(cat ${CM_ETC_HOSTS} | awk "/${rdc_host}/{print \$1}")
        info=$(sqlite3 /var/cm/data/cm_node.db "SELECT id,ipaddr FROM record_t WHERE name='${rem_host}'")
        snid=${info%%|*}
        sipaddr=${info##*|}
        netmask_slave=$(timeout 2 ceres_exec $sipaddr "zfs get -H rdc:ip_owner $path | awk '{print \$3}' | cut -d ',' -f 3")
        [ $? -ne 0 ]&&return $CM_FAIL
        echo "$path" "$nic" "$snid" "$rdc_ip_master" "$rdc_ip_slave" "${netmask_master}" "${netmask_slave}" "${flag}"
        tmp=`expr $tmp + 2`
    done
    return $CM_OK
}

# param: offset total
# echo: path nic snid rdc_ip_master rdc_ip_slave netmask_master netmask_slave
# 只会返回该节点双活的箭头符号是->的信息(主端)
function cm_cnm_san_seki_only_getbatch
{
    local line=($(sndradm -P | awk '{a[NR]=$0;if($0~/set: 2active/ && a[NR-1]~/->/ && $0~/mode: async/){print a[NR-1]}}' | awk '{print $1,$3}'))
    cm_cnm_san_seki_only_getbatch_comm $1 $2 "${line[*]}" "ASYNC"
    line=($(sndradm -P | awk '{a[NR]=$0;if($0~/set: 2active/ && a[NR-1]~/->/ && $0~/mode: sync/){print a[NR-1]}}' | awk '{print $1,$3}'))
    cm_cnm_san_seki_only_getbatch_comm $1 $2 "${line[*]}" "SYNC"
    return $?
}

#param: path [slave]
function cm_cnm_san_seki_only_delete
{
    local set=
    local line=
    if [ ! -z $2 -a $2 == slave ]; then
        line=($(sndradm -P | awk '{a[NR]=$0;if($0~/set: 2active,/ && a[NR-1]~/<-/){print a[NR-1]}}' | sed 's/<-//' | grep -w ${1}))
        if [ ${#line[@]} -ne 2 ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}] not find the 2active path"
            return $CM_ERR_CNM_SAN_SEKI_NO_RECORD
        fi
        set=${line[1]}:${line[0]}
    else
        line=($(sndradm -P | awk '{a[NR]=$0;if($0~/set: 2active,/ && a[NR-1]~/->/){print a[NR-1]}}' | sed 's/->//' | grep -w ${1}))
        if [ ${#line[@]} -lt 2 ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}] not find the 2active path"
            return $CM_ERR_CNM_SAN_SEKI_NO_RECORD
        fi
        set=${line[0]}:${line[1]}
        local rhosts=${line[1]%%:*}
        local slave_rdc=$(cat ${CM_ETC_HOSTS} | awk "/${rhosts}/{print \$1}")
        timeout 10 ceres_exec $slave_rdc "/var/cm/script/cm_cnm_lun.sh 'seki_slave_delete' $1"
    fi
/usr/bin/expect <<EOF
set timeout 10
spawn sndradm -d $set
expect "]:"
send "y\r"
interact
expect eof
EOF

    local lname=$(cat /etc/nodename)
    local lsbbid=$(sqlite3 /var/cm/data/cm_node.db "SELECT sbbid FROM record_t WHERE name='${lname}'")
    local err_ip=$(sqlite3 /var/cm/data/cm_node.db "SELECT ipaddr FROM record_t WHERE sbbid=${lsbbid} AND name!='${lname}'")
    
    local loc_host=${line[0]%%:*}
    local rem_host=${line[1]%%:*}
     #没有这个池的备份或者双活记录了就把rdc_ip删了
    local cnt=$(sndradm -P | grep -w $loc_host | wc -l) 
    if [ $cnt -eq 0 ]; then 
        cat ${CM_ETC_HOSTS} | awk "{if(\$0~/${loc_host}/){}else{print \$0}}" >${CM_ETC_HOSTS_TMP}
        mv ${CM_ETC_HOSTS_TMP} ${CM_ETC_HOSTS}
        #同时把故障切换节点的hosts文件这条记录给删了
        timeout 1 ceres_exec $err_ip "(cat ${CM_ETC_HOSTS} | awk '{if(\$0~/${loc_host}/){}else{print \$0}}' >${CM_ETC_HOSTS_TMP};mv ${CM_ETC_HOSTS_TMP} ${CM_ETC_HOSTS})"
        local pool=$(echo ${line[0]##*:} | cut -d '/' -f 5)
        #同时把网卡的rdc_ip解绑
        local rdc_fuck=$(zfs get -H rdc:ip_owner $pool | awk '{print $3}' | cut -d ',' -f 1-2)
        local interface=${rdc_fuck%%,*}
        local rdc_ip=${rdc_fuck##*,}
        ifconfig $interface removeif $rdc_ip 2>/dev/null
        #清除池的rdc:ip
        zfs set rdc:ip_owner=- $pool 2>/dev/null
    fi
    local cnt=$(sndradm -P | grep -w $rem_host | wc -l)
    if [ $cnt -eq 0 ]; then 
        cat ${CM_ETC_HOSTS} | awk "{if(\$0~/${rem_host}/){}else{print \$0}}" >${CM_ETC_HOSTS_TMP}
        mv ${CM_ETC_HOSTS_TMP} ${CM_ETC_HOSTS}
        timeout 1 ceres_exec $err_ip "(cat ${CM_ETC_HOSTS} | awk '{if(\$0~/${rem_host}/){}else{print \$0}}' >${CM_ETC_HOSTS_TMP};mv ${CM_ETC_HOSTS_TMP} ${CM_ETC_HOSTS})"
    fi
    
    #把数据卷的视图给删除，如果在建立备份之前数据卷已经映射出去，
    #那么会造成该卷不可被外部访问
    #local lu=$(stmfadm list-all-luns | awk "{if(\$2==\"$1\"){print \$1}}")
    #[ ! -z $lu ]&&stmfadm remove-view -a -l $lu
    return $CM_OK
}

# 配置备份avs服务
# param: $path_1 $path_2 $bmpath_1 $bmpath_2 $host_1 $host_2 $flag $issync (SYNC,ASYNC)
# cnt: 9
function cm_cnm_san_backup_avs_conf
{
    local opt="-Ex"
    local sync="sync"
    [ "$8" == "ON" ]&&opt="-ex"
    [ "$9" == "ASYNC" ]&&sync="async"
    if [ $7 == remote ]; then
    local iRet=
# 配置avs服务
/usr/bin/expect <<EOF
set timeout 5
spawn sndradm $opt $6 /dev/zvol/rdsk/$2 /dev/zvol/rdsk/$4 $5 /dev/zvol/rdsk/$1 /dev/zvol/rdsk/$3 ip $sync
expect "]:"
send "y\r"
interact
expect eof
catch wait ret
exit [lindex \$ret 3]
EOF
    iRet=$?
    else
/usr/bin/expect <<EOF
set timeout 5
spawn sndradm $opt $5 /dev/zvol/rdsk/$1 /dev/zvol/rdsk/$3 $6 /dev/zvol/rdsk/$2 /dev/zvol/rdsk/$4 ip $sync
expect "]:"
send "y\r"
interact
expect eof
catch wait ret
exit [lindex \$ret 3]
EOF
    iRet=$?
    fi
    return $iRet
}

# param: $path $bmpath $vol_lu "$nic" "$ip_1""$loc_rdc" "$ip_2" "$rem_rdc" $name $rname $path_2 $bmpath_2 $issync $netmask (SYNC,ASYNC)
# cnt: 15
function cm_cnm_san_backup_main_each
{   
    cm_cnm_san_seki_pre_main $1 $2 "$3" "$4" "$5" "$6" "$7" "$8" "$9" "${10}" "${14}"
    local rs=$?
    if [ $rs -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] prepare fail"
        return $rs
    fi
    # 配置和启动AVS服务
    cm_cnm_san_backup_avs_conf $1 ${11} $2 ${12}  "$9" "${10}" 'remote' "${13}" ${15}
    if [ $? -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] config avs service"
        return $CM_ERR_CNM_SAN_SEKI_CONF_AVS
    fi
    return $CM_OK
}

# nid_1 rip path_1 $path_2 nic rdc_ip_1 rdc_ip_2 "issync" "netmask_1" "netmask_2" "(SYNC,ASYNC)" 
# cnt: 11
function cm_cnm_san_backup_main
{
    local timestamp=`date "+%Y%m%d%H%M%S"`  
    local path_1=$3
    local path_2=$4
    local bmpath_1=${path_1%/*}/bitmap_vol_${timestamp}_bit0
    local bmpath_2=${path_2%/*}/bitmap_vol_${timestamp}_bit0
    local name=$(hostname)_$timestamp
    local rname=`timeout 1 ceres_exec $2 "hostname"`_$timestamp 
    
    # 检查两个卷size是否相同
    local vol_sz_1=$(zfs list -H -o volsize $path_1)
    local vol_sz_2=$(timeout 1 ceres_exec $2 "zfs list -H -o volsize $path_2")
    if [ $vol_sz_1 != $vol_sz_2 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] different volume size"
        return $CM_ERR_CNM_SAN_SEKI_DIFF_VOL_SZ
    fi

    cm_cnm_lun_backup_check $path_1 $2 $path_2
    local vol_stat=$?
    if [ $vol_stat -ne $CM_OK ];then
        return $vol_stat
    fi
    local loc_rdc=
    local rem_rdc=
    if [ -z $6 ]; then
        loc_rdc=$(zfs get -H rdc:ip_owner $path_1 | awk '{print $3}' | cut -d ',' -f 2)
        name=$(cat ${CM_ETC_HOSTS} | grep -w $loc_rdc | awk '{print $2}' | head -1)
    fi
    if [ -z $7 ]; then
        rem_rdc=$(timeout 2 ceres_exec $2 "zfs get -H rdc:ip_owner ${path_2} | awk '{print \$3}' | cut -d ',' -f 2")
        rname=$(timeout 1 ceres_exec $2 "cat ${CM_ETC_HOSTS} | grep -w $rem_rdc | awk '{print \$2}' | head -1")
    fi
    
    local vol_lu_1=$(stmfadm list-all-luns | grep -w $path_1 | awk '{print $1}')
    local vol_lu_2=$(timeout 2 ceres_exec $2 "stmfadm list-all-luns | grep -w $path_2 | awk '{print \$1}'")
    cm_cnm_san_seki_pre_main $3 $bmpath_1 "$vol_lu_1" "$5" "$6" "$loc_rdc" "$7" "$rem_rdc" "$name" "$rname" "$9"
    local rs=$?
    if [ $rs -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] prepare fail"
        return $rs
    fi
    [ -z "${name}" -o -z "${rname}" ]&&return $CM_FAIL
    #$path $bmpath $vol_lu "$nic" "$ip_1""$loc_rdc" "$ip_2" "$rem_rdc" $name $rname $path_2 $bmpath_2 $issync $netmask_1 (SYNC, ASYNC) $interval
    timeout 10 ceres_exec $2 "/var/cm/script/cm_cnm_lun.sh backup_main_each $path_2 $bmpath_2 '${vol_lu_2}' '$5' '$7' '${rem_rdc}' '$6' '${loc_rdc}' '${rname}' '${name}' ${path_1} ${bmpath_1} '$8' '${10}' '${11}'"
    rs=$?
    if [ $rs -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] remote config fail"
        return $rs
    fi
    
    # 配置和启动AVS服务
    cm_cnm_san_backup_avs_conf $path_1 $path_2 $bmpath_1 $bmpath_2 $name $rname 'local' "$8" "${11}"
    if [ $? -ne 0 ]; then
        CM_LOG "[${FUNCNAME}:${LINENO}] config avs service"
        return $CM_ERR_CNM_SAN_SEKI_CONF_AVS
    fi
    return $CM_OK
}

# param: none
# echo:  count
function cm_cnm_san_backup_count
{
    local cnt=$(sndradm -P | awk '{a[NR]=$0;if($0~/set: backup/ && a[NR-1]~/->/){print}}' | wc -l)
    echo $cnt
    return $?
}

# param:offset total linearr flag(ASYNC, SYNC) sync_gap
function cm_cnm_san_backup_getbatch_comm
{
    local flag=$4
    local line=($3)     #要加括号
    local gaps=($5)
    local tmp=
    ((tmp=2*$1))
    local total=
    ((total=2*$2))
    local end=
    ((end=$tmp+$total))
    local path=
    local nic=
    local slave_ip=
    local rdc_ip_master=
    local rdc_ip_slave=
    local path_slave=
    local netmask_master=
    local netmask_slave=
    local igap=0
    while [[ $tmp -lt $end && ! -z ${line[$tmp]} ]]; do
        path=$(echo ${line[$tmp]##*:} | sed 's/\/dev\/zvol\/rdsk\///')
        path_slave=$(echo ${line[$tmp+1]##*:} | sed 's/\/dev\/zvol\/rdsk\///')
        local rdc_info_1=$(zfs get -H rdc:ip_owner $path | awk '{print $3}')
        rdc_ip_master=$(echo $rdc_info_1 | cut -d ',' -f 2)
        netmask_master=${rdc_info_1##*,}
        nic=${rdc_info_1%%,*}
        local rdc_info_2=${line[$tmp+1]}
        local rdc_host=${rdc_info_2%%:*}
        rdc_ip_slave=$(cat ${CM_ETC_HOSTS} | awk "/${rdc_host}/{print \$1}")
        slave_ip=$(timeout 2 ceres_exec $rdc_ip_slave "/var/cm/script/cm_shell_exec.sh cm_get_localmanageip")
        netmask_slave=$(timeout 2 ceres_exec $rdc_ip_slave "zfs get -H rdc:ip_owner $path_slave | awk '{print \$3}' | cut -d ',' -f 3")
        echo "$path" "$nic" "$slave_ip" "$rdc_ip_master" "$rdc_ip_slave" "$path_slave" "${netmask_master}" "${netmask_slave}" "${flag}" "${gaps[$igap]}"
        tmp=`expr $tmp + 2`
        igap=`expr $igap + 1`
    done
    return $CM_OK
}

# param: offset total
# echo: path nic snid rdc_ip_master rdc_ip_slave netmask_master netmask_slave sync_gap
# 只会返回该节点备份的箭头符号是->的信息(主端)
function cm_cnm_san_backup_getbatch
{
    local line=$(sndradm -P | awk '{a[NR]=$0;if($0~/set: backup/ && a[NR-1]~/->/ && $0~/mode: async/){print a[NR-1],$17}}'|awk '{print $1,$3}')
    local gap=($(sndradm -P | awk '{a[NR]=$0;if($0~/set: backup/ && a[NR-1]~/->/ && $0~/mode: async/){print $17}}'|sed 's/,//'))
    cm_cnm_san_backup_getbatch_comm $1 $2 "${line[*]}" "ASYNC" "${gap[*]}"
    line=$(sndradm -P | awk '{a[NR]=$0;if($0~/set: backup/ && a[NR-1]~/->/ && $0~/mode: sync/){print a[NR-1],$17}}'|awk '{print $1,$3}')
    gap=($(sndradm -P | awk '{a[NR]=$0;if($0~/set: backup/ && a[NR-1]~/->/ && $0~/mode: sync/){print $17}}'|sed 's/,//'))
    cm_cnm_san_backup_getbatch_comm $1 $2 "${line[*]}" "SYNC" "${gap[*]}"
    return $?
}

#param: master: path_1 $rip $path_2
#       slave: path_2 'slave'
function cm_cnm_san_backup_delete
{
    local set=
    local line=
    local master_set=
    local slave_set=
    if [ ! -z $2 -a $2 == slave ]; then
        line=($(sndradm -P | awk '{a[NR]=$0;if($0~/set: backup,/ && a[NR-1]~/<-/){print a[NR-1]}}' | sed 's/<-//' | grep -w ${1}))
        if [ ${#line[@]} -ne 2 ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}] not find the backup path"
            return $CM_ERR_CNM_SAN_SEKI_NO_RECORD
        fi
        master_set=${line[1]}
        slave_set=${line[0]}
    else
        line=($(sndradm -P | awk '{a[NR]=$0;if($0~/set: backup,/ && a[NR-1]~/->/){print a[NR-1]}}' | sed 's/->//' | grep -w ${1}))
        if [ ${#line[@]} -lt 2 ]; then
            CM_LOG "[${FUNCNAME}:${LINENO}] not find the backup path"
            return $CM_ERR_CNM_SAN_SEKI_NO_RECORD
        fi
        master_set=${line[0]}
        for ((i=1; i<${#line[@]}; i++)); do
            local tmp=$(echo ${line[$i]} | cut -d '/' -f 5-)
            [[ "$tmp" == "$3" ]]&&slave_set=${line[$i]}
        done
        [ -z $slave_set ]&&return $CM_ERR_CNM_SAN_SEKI_NO_RECORD
        timeout 10 ceres_exec $2 "/var/cm/script/cm_cnm_lun.sh 'backup_slave_delete' $3"
    fi
    set=${master_set}:${slave_set}
    local err=$(sndradm -n -d $set 2>&1 | cut -d ' ' -f 8) 
    [ ! -z $err -a $err == local ]&&return $CM_ERR_CNM_SAN_SEKI_BOTH_LOCAL
    
    local lname=$(cat /etc/nodename)
    local lsbbid=$(sqlite3 /var/cm/data/cm_node.db "SELECT sbbid FROM record_t WHERE name='${lname}'")
    local err_ip=$(sqlite3 /var/cm/data/cm_node.db "SELECT ipaddr FROM record_t WHERE sbbid=${lsbbid} AND name!='${lname}'")
    
    local loc_host=${line[0]%%:*}
    local rem_host=${slave_set%%:*}
    [ $2 == slave ]&&rem_host=${master_set%%:*}
    #没有这个池的备份或者双活记录了就把rdc_ip删了
    local cnt=$(sndradm -P | grep -w $loc_host | wc -l) 
    if [ $cnt -eq 0 ]; then 
        cat ${CM_ETC_HOSTS} | awk "{if(\$0~/${loc_host}/){}else{print \$0}}" >${CM_ETC_HOSTS_TMP}
        mv ${CM_ETC_HOSTS_TMP} ${CM_ETC_HOSTS}
        #同时把故障切换节点的hosts文件这条记录给删了
        timeout 2 ceres_exec $err_ip "(cat ${CM_ETC_HOSTS} | awk '{if(\$0~/${loc_host}/){}else{print \$0}}' >${CM_ETC_HOSTS_TMP};mv ${CM_ETC_HOSTS_TMP} ${CM_ETC_HOSTS})"
        local pool=$(echo ${line[0]##*:} | cut -d '/' -f 5)
        #同时把网卡的rdc_ip解绑
        local rdc_fuck=$(zfs get -H rdc:ip_owner $pool | awk '{print $3}' | cut -d ',' -f 1-2)
        local interface=${rdc_fuck%%,*}
        local rdc_ip=${rdc_fuck##*,}
        ifconfig $interface removeif $rdc_ip 2>/dev/null
        zfs set rdc:ip_owner=- $pool 2>/dev/null
    fi
    local cnt=$(sndradm -P | grep -w $rem_host | wc -l)
    if [ $cnt -eq 0 ]; then 
        cat ${CM_ETC_HOSTS} | awk "{if(\$0~/${rem_host}/){}else{print \$0}}" >${CM_ETC_HOSTS_TMP}
        mv ${CM_ETC_HOSTS_TMP} ${CM_ETC_HOSTS}
        #同时把故障切换节点的hosts文件这条记录给删了
        timeout 2 ceres_exec $err_ip "(cat ${CM_ETC_HOSTS} | awk '{if(\$0~/${rem_host}/){}else{print \$0}}' >${CM_ETC_HOSTS_TMP};mv ${CM_ETC_HOSTS_TMP} ${CM_ETC_HOSTS})"
    fi
    
    #把数据卷的视图给删除，如果在建立备份之前数据卷已经映射出去，
    #那么会造成该卷不可被外部访问
    #local lu=$(stmfadm list-all-luns | awk "{if(\$2==\"$1\"){print \$1}}")
    #[ ! -z $lu ]&&stmfadm remove-view -a -l $lu 2>/dev/null
    return $CM_OK
}

# param: set interval
function cm_cnm_san_backup_set_interval
{
    [ -z $1 ]&&return $CM_PARAM_ERR
    [ $2 -eq 0 ]&&return $CM_PARAM_ERR
    sndradm -T $2 -n $1
    return $?
}

#==================================================================================
#==================================================================================
#                                  LUN_MIRROR功能管理
# 使用说明:
# 批量查询  : cm_cnm_lun.sh mirror_getbatch <offset> <total>
# 记录数查询: cm_cnm_lun.sh mirror_count
# 添加记录  : cm_cnm_lun.sh mirror_create  <snid> <path> <nic> <rdc_ip_master> <rdc_ip_slave>
# 删除记录  : cm_cnm_lun.sh mirror_delete  <path>
#==================================================================================
#==================================================================================

#==================================================================================
#                                  批量查询
# 输入参数:
#       <offset>
#       <total>
# 回显数据:
# path nic snid rdc_ip_master rdc_ip_slave
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_lun_mirror_getbatch()
{
    cm_cnm_san_seki_only_getbatch "$@"
    return $?
}

#==================================================================================
#                                  批量记录数
# 输入参数:
# 回显数据:
# count
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_lun_mirror_count()
{
    cm_cnm_san_seki_only_count
    return $?
}

#==================================================================================
#                                  添加
# 输入参数:
#   <snid> <path> <nic> <rdc_ip_master> <rdc_ip_slave> <issync> <netmask_1> <netmask_2> <SYNC,ASYNC>
# 回显数据:
# none
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_lun_mirror_create()
{
    local ip=`cm_get_localmanageip`
    local nid=$(sqlite3 /var/cm/data/cm_node.db "SELECT id FROM record_t WHERE ipaddr='${ip}'")
    [ -z $nid ]&&return $CM_FAIL
    cm_cnm_main seki_only $nid "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9"
    return $?
}

#==================================================================================
#                                  删除
# 输入参数:
# <path>
# 回显数据:
# none
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_lun_mirror_delete()
{
    cm_cnm_san_seki_only_delete $1
    return $?
}

#==================================================================================
#==================================================================================
#                                  LUN_BACKUP功能管理
# 使用说明:
# 批量查询  : cm_cnm_lun.sh backup_getbatch <offset> <total>
# 记录数查询: cm_cnm_lun.sh backup_count
# 添加记录  : cm_cnm_lun.sh backup_create  <sip> <path> <nic> <rdc_ip_master> <rdc_ip_slave> <path_dest>
# 删除记录  : cm_cnm_lun.sh backup_delete  <path>
#==================================================================================
#==================================================================================

# param: set action
function cm_cnm_san_backup_update_sync
{
    local action=$2
    local set=$1
    local opt=
    case $action in
        FULL_SYNC):
            opt="-m"
            ;;
        UPDATE_SYNC):
            opt="-u"
            ;;
        REV_FULL_SYNC):
            opt="-m -r"
            ;;
        REV_UPDATE_SYNC):
            opt="-u -r"
            ;;
        *):
            return $CM_ERR_NOT_SUPPORT
            ;;
    esac
    sndradm -a off -n $set
    sndradm -l -n $set  
    sndradm "$opt" -n $set
    sndradm -w -n $set
    sndradm -a on -n $set
    return $?
}

#                                   1                             2       3             4           5 
#(SET_TIME,FULL_SYNC,UPDATE_SYNC,REV_FULL_SYNC,REV_UPDATE_SYNC) <path> <ip_slave> <path_slave> <interval>
function cm_cnm_san_backup_update
{
    local action=$1
    # SAN备份 && 异步 && 主端
    local line=($(sndradm -P | awk '{a[NR]=$0;if($0~/set: backup/ && a[NR-1]~/->/){print a[NR-1]}}' | awk '{print $1,$3}'))
    local length=${#line[@]}
    if [ $length -lt 2 ]; then  # 成双，小于2没有SAN备份
        CM_LOG "[${FUNCNAME}:${LINENO}] have no backup record"
        return $CM_ERR_CNM_SAN_SEKI_NO_RECORD
    fi
    local rname=$(timeout 2 ceres_exec $3 "hostname")
    [ -z "$rname" ]&&return $CM_FAIL
    local cnt=0
    local master_set=   # set前半部分
    local slave_set=    # set后半部分
    local set=
    for ((cnt=0; cnt<${length}; cnt=${cnt}+2)); do
        echo ${line[$cnt]} | grep -w $2
        [ $? -eq 0 ]&&master_set="${line[$cnt]}"
        echo ${line[1+$cnt]} | grep -w $4
        if [ $? -eq 0 ]; then
            local fname=${line[1+$cnt]%%:*}
            local hname=${fname%%_*}
            [ $rname == $hname ]&&slave_set=${line[1+$cnt]}
        fi
    done
    [ ! -z "${master_set}" -a ! -z "${slave_set}" ]&&set=${master_set}:${slave_set}
    case $action in
        SET_TIME):
            [ $5 -eq 0 ]&&return $CM_PARAM_ERR
            cm_cnm_san_backup_set_interval $set $5  # set interval
            return $?
            ;;
        *):
            cm_cnm_san_backup_update_sync $set $action
            return $?
            ;;
    esac
}

#==================================================================================
#                                  批量查询
# 输入参数:
#       <offset>
#       <total>
# 回显数据:
# path nic slave_ip rdc_ip_master rdc_ip_slave path_slave
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_lun_backup_getbatch()
{
    cm_cnm_san_backup_getbatch "$@"
    return $?
}

#==================================================================================
#                                  批量记录数
# 输入参数:
# 回显数据:
# count
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_lun_backup_count()
{
    cm_cnm_san_backup_count
    return $?
}

#==================================================================================
#                                  添加
# 输入参数:
#   1       2       3       4               5               6       7       8           9       10
#   <sip> <path> <nic> <rdc_ip_master> <rdc_ip_slave> <path_dest> issync netmask_1 netmask_2 (SYNC,ASYNC)
# 回显数据:
# none
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_lun_backup_create()
{
    local ip=`cm_get_localmanageip`
    local nid=$(sqlite3 /var/cm/data/cm_node.db "SELECT id FROM record_t WHERE ipaddr='${ip}'")
    [ -z $nid ]&&return $CM_FAIL
    cm_cnm_main backup $nid "$1" "$2" "$6" "$3" "$4" "$5" "$7" "$8" "$9" "${10}"
    return $?
}

#==================================================================================
#                                  修改
# 输入参数:
#                                   1                             2       3             4           5 
#(SET_TIME,FULL_SYNC,UPDATE_SYNC,REV_FULL_SYNC,REV_UPDATE_SYNC) <path> <ip_slave> <path_slave> <interval>
# 回显数据:
# none
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================

#==================================================================================
#                                  删除
# 输入参数:
# <path> <rip> <dest_path>
# 回显数据:
# none
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_lun_backup_delete()
{
    cm_cnm_san_backup_delete $@
    return $?
}

#param:<slave_path>
function cm_cnm_lun_backup_get_slave_bitmap()
{
    [ -z "$1" ]&&return $CM_FAIL
    local bitmap=$(kstat sndr::setinfo | awk '/\/dev\/zvol\/rdsk/{print $0}' | awk "{a[NR]=\$0;if(\$1 == \"secondary_vol\" && \$2 == \"/dev/zvol/rdsk/$1\"){print a[NR-2]}}" | awk '{print $2}')
    if [ -z "$bitmap" ]; then 
        CM_LOG "[${FUNCNAME}:${LINENO}] not find the slave bitmap"
        return $CM_FAIL
    fi
    echo $bitmap
    return $CM_OK
}

# param: <master_path> <slave_ip> <slave_path>
function cm_cnm_lun_backup_get_prog()
{
    local tmparr=($(dsstat -m sndr -s /dev/zvol/rdsk/$1 | awk 'NR >= 2{print $4}'))
    if [ ${#tmparr[@]} -eq 0 ]; then 
        echo "200"  # cm_exec_int无法返回真实的错误值，通过大于100的值代表错误
        return $CM_OK
    fi
    # 没有一个LUN到不同节点相同LUN的备份
    if [ ${#tmparr[@]} -eq 2 ]; then 
        echo `expr 100 - ${tmparr[0]%%.*}`  # 取整
        return $CM_OK
    fi
    #有一个LUN到多个LUN备份
    local bitmap_path=$(timeout 3 ceres_exec $2 "/var/cm/script/cm_cnm_lun.sh slave_get_bitmap $3")
    [ -z "$bitmap_path" ]&&return $CM_FAIL
    bitmap_path=${bitmap_path:0-15:15}
    local i=0
    local sets=$(dsstat -m sndr -s /dev/zvol/rdsk/$1 | awk 'NR>=2{print $1}')
    for((; i<${#sets[@]}; i+=2)); do
        #位图卷每个配对都是不同的，根据位图卷区分dsstat的信息
        if [ "${bitmap_path}" == "${sets[1+$i]}" ]; then
            break
        fi
    done
    [ $i -lt ${#sets[@]} ]&&echo `expr 100 - ${tmparr[$i]%%.*}`
    return $CM_OK
}

function cm_cnm_main()
{
    action=$1
    case $action in
        seki_only):
            [ $# -lt 11 ]&&return $CM_PARAM_ERR
            # nid_1 nid_2 path (nic ip_1 ip_2 issync netmask_1 netmask_2)可能为"" (SYNC,ASYNC)
            cm_cnm_san_seki_only_main $2 $3 $4 "$5" "$6" "$7" "$8" "$9" "${10}" "${11}"
            return $?
            ;;
        backup):
            [ $# -lt 12 ]&&return $CM_PARAM_ERR
            cm_cnm_san_backup_main $2 $3 $4 $5 "$6" "$7" "$8" "$9" "${10}" "${11}" "${12}" 
            return $?
            ;;
        seki_main_each):
            [ $# -lt 14 ]&&return $CM_PARAM_ERR
            #$path $bmpath $vol_lu "$nic" "$ip_1""$loc_rdc" "$ip_2" "$rem_rdc" $name $rname $issync $netmask_1 (SYNC,ASYNC)
            cm_cnm_san_seki_only_main_each $2 $3 $4 "$5" "$6" "$7" "$8" "$9" "${10}" "${11}" "${12}" "${13}" "${14}"
            return $?
            ;;
        backup_main_each):
            [ $# -lt 16 ]&&return $CM_PARAM_ERR
            # $path $bmpath $vol_lu "$nic" "$ip_1" "$loc_rdc" "$ip_2" "$rem_rdc" $name $rname $path_2 $bmpath_2 $issync $netmask_1 (SYNC, ASYNC)
            cm_cnm_san_backup_main_each $2 $3 "$4" "$5" "$6" "$7" "$8" "$9" ${10} ${11} ${12} ${13} "${14}" "${15}" ${16}
            return $?
            ;;
        seki_slave_delete):
            cm_cnm_san_seki_only_delete $2 'slave'
            return $?
            ;;
        backup_slave_delete):
            cm_cnm_san_backup_delete $2 'slave'
            return $?
            ;;
        mirror_getbatch)
            cm_cnm_lun_mirror_getbatch "$2" "$3"
            return $?
            ;;
        mirror_count):
            cm_cnm_lun_mirror_count
            return $?
            ;;
        mirror_create):
            #7:issync 8:netmask_1 9:netmask_2 10:(SYNC,ASYNC)
            cm_cnm_lun_mirror_create "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9" "${10}"
            return $?
            ;;
        mirror_delete):
            cm_cnm_lun_mirror_delete "$2"
            return $?
            ;;
        backup_getbatch):
            cm_cnm_lun_backup_getbatch "$2" "$3"
            return $?
            ;;
        backup_count):
            cm_cnm_lun_backup_count
            return $?
            ;;
        backup_create):
            # 8:issync 9:netmask_1 10:netmask_2 11:(SYNC,ASYNC)
            cm_cnm_lun_backup_create "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9" "${10}" "${11}"
            return $?
            ;;
        backup_update):
            # (SET_TIME,FULL_SYNC,UPDATE_SYNC,REV_FULL_SYNC,REV_UPDATE_SYNC) <path> <ip_slave> <path_slave> <interval>
            cm_cnm_san_backup_update "$2" "$3" "$4" "$5" "$6"
            return $?
            ;;
        backup_delete):
            cm_cnm_lun_backup_delete "$2" "$3" "$4"
            return $?
            ;;
        backup_prog):
            [ $# -lt 2 ]&&return $CM_PARAM_ERR
            local param="$2"
            local master_path=${param%%|*}
            local slave_ip=$(echo "$param" | cut -d '|' -f 2)
            local slave_path=$(echo "$param" | cut -d '|' -f 3)
            #param: <master_path> <slave_ip> <slave_path>
            cm_cnm_lun_backup_get_prog "$master_path" "$slave_ip" "$slave_path"
            return $?
            ;;
        slave_get_bitmap):
            [ $# -lt 2 ]&&return $CM_PARAM_ERR
            cm_cnm_lun_backup_get_slave_bitmap "$2"
            return $?
            ;;
        mirror_check):
            cm_cnm_lun_mirror_local_check $2
            return $?
            ;;
        backup_check):
            cm_cnm_lun_backup_local_check $2
            return $?
            ;;
        *):
            return $CM_ERR_NOT_SUPPORT
            ;;
    esac
}

# ---------------------------- main ------------------------------- 
cm_cnm_main "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9" "${10}" "${11}" "${12}" "${13}" "${14}" "${15}" "${16}" "${17}" "${18}" "${19}" "${20}" "${21}"
exit $?
