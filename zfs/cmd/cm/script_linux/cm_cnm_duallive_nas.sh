#!/bin/bash

source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

CM_CNM_DUALLIVE_MASTER_SSHIP_ERR=231
CM_CNM_DUALLIVE_SLAVE_SSHIP_ERR=232
CM_CNM_DUALLIVE_MASTER_BK_SSHIP_ERR=233
CM_CNM_DUALLIVE_SLAVE_BK_SSHIP_ERR=234

CM_CNM_DUALLVE_SH=/var/cm/script/cm_cnm_duallive_nas.sh
CM_CNM_DUALLIVE_INI=/etc/cm_cnm_duallive.ini
config_path=/etc/duallive.config

cm_ss3='$3'

function get_hid_from_nid()
{
    local nid=$1
    local ret=`sqlite3 /var/cm/data/cm_node.db "select idx from record_t WHERE id=$nid" 2>>/dev/null`
    if [ "$ret"x == ""x ];then
        ret="none"
    fi
    echo $ret
}

function get_sbbid_from_nid()
{
    local nid=$1
    local ret=`sqlite3 /var/cm/data/cm_node.db "select sbbid from record_t WHERE id=$nid" 2>>/dev/null`
    if [ "$ret"x == ""x ];then
        ret="none"
    fi
    echo $ret
}

function get_nids_from_sbbid()
{
    local sbbid=$1
    local ret=`sqlite3 /var/cm/data/cm_node.db "select id from record_t WHERE sbbid=$sbbid" 2>>/dev/null`
    if [ "$ret"x == ""x ];then
        ret="none none"
    fi
    echo $ret
}

function get_brother_nid()
{
    local nid=$1
    local ret=`sqlite3 /var/cm/data/cm_node.db "select id from record_t WHERE id<>$nid AND sbbid IN (SELECT sbbid FROM record_t WHERE id=$nid)" 2>>/dev/null`
    if [ "$ret"x == ""x ];then
        ret="none"
    fi
    echo $ret
}

function get_nid_from_hid()
{
    local hid=$1
    local ret=`sqlite3 /var/cm/data/cm_node.db "select id from record_t WHERE idx=$hid" 2>>/dev/null`
    if [ "$ret"x == ""x ];then
        ret="none"
    fi
    echo $ret
}


function get_ip_from_nid()
{
    local nid=$1
    local ret=`sqlite3 /var/cm/data/cm_node.db "select ipaddr from record_t WHERE id=$nid" 2>>/dev/null`
    if [ "$ret"x == ""x ];then
        ret="none"
    fi
    echo $ret
}


function cm_cnm_duallive_cheak_fsid()
{
    local fsid=$1
    if [ "$fsid"x == "0"x ]||[ "$fsid"x == "17261658112"x ]|| [ "$fsid"x == ""x ] || [ "$fsid"x == "-"x ] || [ "$fsid"x == "connect_fail"x ];then #if [ "$fsid"x == "0"x ];then
        CM_LOG "[${FUNCNAME}:${LINENO}]fsid=$fsid err"
        return $CM_PARAM_ERR
    fi
    return $CM_OK
}

function get_sship_from_ntcd()
{
    local ntcd=$1
    echo $ntcd > test
    local ret=$(ifconfig $ntcd|grep inet|awk '{print $2}')
    if [ "$ret"x == ""x ];then
        ret="none"
    fi
    echo $ret
}

function get_sship_from_nid_and_ntcd()
{
    local nid=$1
    local ntcd=$2
    local ip=`get_ip_from_nid $nid`
    local rets=(`ceres_exec $ip "ifconfig $ntcd|grep inet"`)
    local ret=${rets[1]}
    CM_LOG "[${FUNCNAME}:${LINENO}]$ret"
    if [ "$ret"x == ""x ];then
        ret="none"
    fi
    echo $ret
}

function get_masknum_form_netmask()
{
    local netmask=$1
    local masknum=0
    OLD_IFS="$IFS" 
    IFS="."
    local arr=($netmask)
    IFS="$OLD_IFS"
    for num in ${arr[@]}
    do
    if [ $num -gt 255 ] || [ $num -lt 0 ];then
        CM_LOG "[${FUNCNAME}:${LINENO}]IP NUMBER ERR"
        echo none
        return $CM_PARAM_ERR
    else
        ((masknum=$masknum+`echo "obase=2;$num"|bc|tr -cd 1|wc -c`))
    fi
    done
    echo $masknum
    return $CM_OK
}

function cm_cnm_check_ipaddr()
{
    local ipaddr=$1
    echo $ipaddr|grep "^[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}\.[0-9]\{1,3\}$" > /dev/null
    local ret=$?
    if [ $ret -ne 0 ]
    then
        CM_LOG "[${FUNCNAME}:${LINENO}]IP PARAM ERR"
        return $CM_PARAM_ERR
    fi
    OLD_IFS="$IFS" #IFS is fen ge fu
    IFS="."
    local arr=($ipaddr)
    IFS="$OLD_IFS"
    for num in ${arr[@]}
    do
    if [ $num -gt 255 ] || [ $num -lt 0 ];then
        CM_LOG "[${FUNCNAME}:${LINENO}]IP NUMBER ERR"
        return $CM_PARAM_ERR
    fi
    done
    return $CM_OK
}

CM_CNM_DELEY_S=600 #定义默认延时

#批量转发函数，返回每个节点的错误码，可以设定超时时间(可选，默认3s)
function connect_node_do_script()
{
    local nodes=($1)
    local cmd=$2
    local argv=$3
    local is_reply=$4
    local deley_s=$5

    if [ "$is_reply"x == ""x ];then
        is_reply=$CM_FAIL
    fi

    if [ "$deley_s"x == ""x ];then
        deley_s=$CM_CNM_DELEY_S
    fi

    for n in ${nodes[@]}
    do
        if [ "$n"x == "none"x ];then
            continue
        fi
        local ip=`get_ip_from_nid $n`
        if [ "$ip"x == "none"x ];then
            echo $CM_ERR_TIMEOUT
            continue
        fi
        #echo "ceres_exec $ip \"timeout $deley_s $CM_CNM_DUALLVE_SH $cmd $argv\"" >> test
        timeout $deley_s ceres_exec $ip "$CM_CNM_DUALLVE_SH $cmd $argv"& #超时返回124 CM_CNM_DUALLVE_SH全局定义的脚本名
    done
    
    if [ $is_reply -ne $CM_OK ];then
        return 
    fi
    
    for pid in $(jobs -p)
    do
        wait $pid
        echo $?
        #echo pid:$pid ret:$? >> test
    done
}
#去远端执行命令拿回显
function connect_node_do_cmd()
{
    local nid=$1
    local cmd="$2"
    local deley_s=$3
    if [ $# -ne 3 ];then
        deley_s=$CM_CNM_DELEY_S
    fi

    local ip=`get_ip_from_nid $nid`
    if [ "$ip"x == "none"x ];then
        echo connect_fail
        return
    fi
    #echo "timeout $deley_s ceres_exec $ip \"$cmd\"" >> test
    local ret_str=`timeout $deley_s ceres_exec $ip "$cmd"` #超时返回124 CM_CNM_DUALLVE_SH全局定义的脚本名
    echo $ret_str
    return
}

###############################################################
#
###############################################################

#具体的属性设置 参数：属性值 nas
function STATIC_cm_cnm_duallive_setattr()
{
    local vars=($1) #属性值
    local nas=$2
    local attrs=("sync" "remotefsname" "remoteport" "bNassync" "nastype" "rfsid")
    local index=0
    for attr in ${attrs[@]}
    do
        zfs set $attr=${vars[$index]} $nas 2>>/dev/null
        if [ $? -ne $CM_OK ];then
            CM_LOG "[${FUNCNAME}:${LINENO}]zfs set $attr=${vars[$index]} $nas fail"
            return $CM_PARAM_ERR
        fi
        ((index=$index+1))
    done
    return $CM_OK
}

#重启操作 参数：nas
function STATIC_cm_cnm_duallive_restart()
{
    local nas=$1
    local nas_info=(`zfs list -H -o sharenfs,sharesmb $nas 2>/dev/null`)
    
    zfs umount -f $nas
    if [ $? != $CM_OK ];then
        CM_LOG "[${FUNCNAME}:${LINENO}]zfs umount -f $nas fail"
        return $CM_FAIL
    fi
    zfs mount $nas
    if [ $? != $CM_OK ];then
        CM_LOG "[${FUNCNAME}:${LINENO}]zfs mount $nas fail"
        return $CM_FAIL
    fi
    zfs set sharenfs=${nas_info[0]} $nas
    zfs set sharesmb=${nas_info[1]} $nas
} 

#备份属性设置，重启nas
#参数：<nas类型> <源nas> <目的hid> <目的nas> <同步类型>
function cm_cnm_bankup_setcfg()
{
    local nastype=$1    #nas类型：master/salve
    local srcnas=$2
    local deshid=$3
    local desnas=$4 
    local synctp=$5 #添加同步模式
    local rfsid=$6
    #设置属性
    STATIC_cm_cnm_duallive_setattr "mirror $desnas $deshid $synctp $nastype $rfsid" $srcnas #设置nas文件属性
    if [ $? -ne $CM_OK ];then
        CM_LOG "[${FUNCNAME}:${LINENO}]set node:[$(hostname)] fail"
        return $CM_PARAM_ERR
    fi
    
    #重启
    STATIC_cm_cnm_duallive_restart $srcnas
    if [ $? -ne $CM_OK ];then
        return $CM_ERR_BUSY
    fi
    return $CM_OK
}

function cm_cnm_duallive_swicfg()
{
    local paranum=$#
    if [ $paranum -ne 7 ];then
        CM_LOG "[${FUNCNAME}:${LINENO}] err paranum [$paranum]"
        return $CM_PARAM_ERR
    fi
    local mnid=$1
    local mnas=$2
    local snid=$3
    local snas=$4
    local mwip=$5
    local work_if=$6
    local netmask=$7
    
    #拿到mssh、ssship和两个备份nid
    local mssh_ip=`get_sship_from_nid_and_ntcd $mnid $work_if`
    local sssh_ip=`get_sship_from_nid_and_ntcd $snid $work_if`
    local mbknid=`get_brother_nid $mnid`
    local sbknid=`get_brother_nid $snid`
    
    local mhid=`get_hid_from_nid $mnid`
    local shid=`get_hid_from_nid $snid`
    echo "MASTER_WORKLOAD_IP=$mwip">$config_path
    echo "WORKLOAD_NETMASK=$netmask">>$config_path
    echo "SLAVE_WORKLOAD_IP=0.0.0.100">>$config_path
    echo "WORKLOAD_IF=$work_if">>$config_path
    echo "MHOSTID=$mhid">>$config_path
    echo "M_SSH=$mssh_ip">>$config_path
    local mbkhid=`get_hid_from_nid $mbknid`
    local mbkssh_ip=none
    if [ "$mbkhid"x != "none"x ];then
        mbkssh_ip=`get_sship_from_nid_and_ntcd $mbknid $work_if`
    else
        mbkhid=none
    fi
    echo "MHOSTID_BCK=$mbkhid">>$config_path
    echo "MBCK_SSH=$mbkssh_ip">>$config_path
    echo "SHOSTID=$shid">>$config_path
    echo "S_SSH=$sssh_ip">>$config_path
    local sbkhid=`get_hid_from_nid $sbknid`
    local sbkssh_ip=none
    if [ "$sbkhid"x != "none"x ];then
        sbkssh_ip=`get_sship_from_nid_and_ntcd $sbknid $work_if`
    else
        sbkhid=none
    fi
    echo "SHOSTID_BCK=$sbkhid">>$config_path
    echo "SBCK_SSH=$sbkssh_ip">>$config_path
    echo "MFS=$mnas">>$config_path
    echo "SFS=$snas">>$config_path
    echo "LOG_FILE=/var/adm/duallive.log">>$config_path
    echo "REMOTE_EXEC=clumgt">>$config_path
    return $CM_OK
}

#记录备份关系
#节点列表的格式：主节点：主nas：从节点：从nas：业务ip：业务网卡：子网掩码：本节点角色
function cm_cnm_duallive_record_ini()
{
    local duallive_info=$*
    local mnid=$1
    local snid=$3
    local mbknid=`get_brother_nid $mnid`
    local sbknid=`get_brother_nid $snid`
    local hostid=`cm_get_localid`
    local nid=`get_nid_from_hid $hostid`
    local role=none
    
    case $nid in
        $mnid)
            role=CM_CNM_MASTER
        ;;
        $snid)
            role=CM_CNM_SLAVE
        ;;
        $mbknid)
            role=CM_CNM_MASTER_BK
        ;;
        $sbknid)
            role=CM_CNM_SLAVE_BK
        ;;
        *)
            return $CM_OK #错误的目的IP
        ;;
    esac
    echo "$duallive_info $role" > $CM_CNM_DUALLIVE_INI
    return $OM_OK
}

#删除备份记录
function cm_cnm_duallive_delete_ini()
{
    cat /dev/null > $CM_CNM_DUALLIVE_INI
    return $CM_OK
}

#检测是否nas已经用于其他备份
function cm_cnm_is_allow_create()
{
    local mnid=$1
    local snid=$2
    local mnas="$1 $2"
    local snas="$3 $4"

    local msbbid=`get_sbbid_from_nid $mnid`
    local ssbbid=`get_sbbid_from_nid $snid`
    if [ "$msbbid"x == "$ssbbid"x ];then
        CM_LOG "[${FUNCNAME}:${LINENO}] same sbbid"
        return $CM_PARAM_ERR
    fi
    if [ ! -e $CM_CNM_DUALLIVE_INI ];then
        return $CM_OK
    fi
    #lcoal rezfs get -H remotefsname $srcnas |awk '{print $3}'
    local ini_line1=`cat $CM_CNM_DUALLIVE_INI|grep -w "$mnas"|wc -l`
    local ini_line2=`cat $CM_CNM_DUALLIVE_INI|grep -w "$snas"|wc -l`
    local ini_line=$(($ini_line1 + $ini_line2))
    if [ $ini_line1 -ne 0 ];then
        return $CM_ERR_BUSY
    fi
}

############################################################
#                          create                          #
############################################################
#cm_cnm_duallive.ini 记录备份的创建记录（备份列表）

#master 创建备份 1、检查是否允许被创建 2、设置属性 3、检查错误上报
#对底层需求：双活nas需要被保护，不能通过命令任意修改
function master_create_operation()
{
    #info <主节点：主nas：从节点：从nas：同步方式：业务IP：业务网口：业务掩码>
    local duallive_info=$*
    local mnid=$1
    local mnas=$2
    local snid=$3
    local snas=$4
    local sync=$5
    local mwip=$6
    local mwif=$7
    local netmask=$8
    local rfsid=$(connect_node_do_cmd $snid "zfs get -H fsid $snas|awk '{print $cm_ss3}'")
    
    cm_cnm_duallive_cheak_fsid $rfsid
    local iRet=$?
    if [ $iRet -ne $CM_OK ];then
        CM_LOG"[${FUNCNAME}:${LINENO}] nas[$snas]fsid[$rfsid]err"
        return $iRet
    fi
    
    local shid=`get_hid_from_nid $snid`
    
    cm_cnm_is_allow_create $duallive_info
    if [ $? -ne $CM_OK ];then
        return $CM_ERR_BUSY
    fi

    #判断网卡是否配置IP
    #echo $duallive_info :$mwif :$(get_sship_from_ntcd $work_if) > /var/test
    if [ "$(get_sship_from_ntcd $mwif)"x == "none"x ];then
        return $CM_CNM_DUALLIVE_MASTER_SSHIP_ERR
    fi
    cm_cnm_duallive_swicfg $mnid $mnas $snid $snas $mwip $mwif $netmask

    cm_cnm_bankup_setcfg master $mnas $shid $snas $sync $rfsid
    local iRet=$?
    if [ $iRet -ne $CM_OK ];then
        return $iRet
    fi

    return $CM_OK
}

function slave_create_operation()
{
    local duallive_info=$*
    local mnid=$1
    local mnas=$2
    local snid=$3
    local snas=$4
    local synctype=$5
    local mwip=$6
    local mwif=$7
    local netmask=$8
    local mhid=`get_hid_from_nid $mnid`
    local rfsid=$(connect_node_do_cmd $mnid "zfs get -H fsid $mnas|awk '{print $cm_ss3}'")

    cm_cnm_duallive_cheak_fsid $rfsid
    local iRet=$?
    if [ $iRet -ne $CM_OK ];then
        CM_LOG"[${FUNCNAME}:${LINENO}] nas[$snas]fsid[$rfsid]err"
        return $iRet
    fi

    cm_cnm_is_allow_create $duallive_info
    if [ $? -ne $CM_OK ];then
        return $CM_ERR_BUSY
    fi
    
    if [ "$(get_sship_from_ntcd $mwif)"x == "none"x ];then
        return $CM_CNM_DUALLIVE_SLAVE_SSHIP_ERR
    fi
    cm_cnm_duallive_swicfg $mnid $mnas $snid $snas $mwip $mwif $netmask
    
    cm_cnm_bankup_setcfg slave $snas $mhid $mnas $synctype $rfsid
    local iRet=$?
    if [ $iRet -ne $CM_OK ];then
        return $iRet
    fi

    return $CM_OK
}

function master_bk_create_operation()
{
    local mnid=$1
    local mnas=$2
    local snid=$3
    local snas=$4
    local mwip=$6
    local mwif=$7
    local netmask=$8
    if [ "$(get_sship_from_ntcd $mwif)"x == "none"x ];then
        return $CM_CNM_DUALLIVE_MASTER_BK_SSHIP_ERR
    fi
    cm_cnm_duallive_swicfg $mnid $mnas $snid $snas $mwip $mwif $netmask 

    return $CM_OK
}

function slave_bk_create_operation()
{
    local mnid=$1
    local mnas=$2
    local snid=$3
    local snas=$4
    local mwip=$6
    local mwif=$7
    local netmask=$8
    if [ "$(get_sship_from_ntcd $mwif)"x == "none"x ];then
        return $CM_CNM_DUALLIVE_SLAVE_BK_SSHIP_ERR
    fi
    cm_cnm_duallive_swicfg $mnid $mnas $snid $snas $mwip $mwif $netmask

    return $CM_OK
}

#node执行create中转站
function cm_cnm_duallive_create()
{
    local duallive_info=$*
    local mnid=$1
    local snid=$3
    local mbknid=`get_brother_nid $mnid`
    local sbknid=`get_brother_nid $snid`
    local hostid=`cm_get_localid`
    local nid=`get_nid_from_hid $hostid`
    
    case $nid in
        $mnid)
            master_create_operation $duallive_info
            return $?
        ;;
        $snid)
            slave_create_operation $duallive_info
            return $?
        ;;
        $mbknid)
            master_bk_create_operation $duallive_info
            return $?
        ;;
        $sbknid)
            slave_bk_create_operation $duallive_info
            return $?
        ;;
        *)
            return $CM_OK #错误的目的IP
        ;;
    esac
}

############################################################
#                          delete                          #
############################################################
FS_XML=/tmp/fs.xml
function cm_cnm_delete_operation()
{
    local nas=$1
    local have_nas=$(cat $FS_XML|grep "type=\"filesystem\" name=\"$nas\""|wc -l)
    if [ $have_nas -eq $CM_OK ];then
        return $CM_OK
    fi
    cm_cnm_bankup_setcfg master $nas 9100 none off 0
    local iRet=$?
    if [ $iRet -ne $CM_OK ];then
        return $iRet
    fi
    return $CM_OK
}

function cm_cnm_duallive_delete()
{
    local mnid=$1
    local mnas=$2
    local snid=$3
    local snas=$4
    local mbknid=`get_brother_nid $mnid`
    local sbknid=`get_brother_nid $snid`
    local hostid=`cm_get_localid`
    local nid=`get_nid_from_hid $hostid`

    if [ $nid -eq $mnid ]||[ $nid -eq $mbknid ];then
        cm_cnm_delete_operation $mnas
    elif [ $nid -eq $snid ]||[ $nid -eq $sbknid ];then
        cm_cnm_delete_operation $snas
    fi
    return $CM_OK
}

#获取nas当前的nid
function get_current_nid_form_nas()
{
    local sbbid=$1
    local origin_nid=$2
    local nas=$3
    local nids=(`get_nids_from_sbbid $sbbid`)
    if [ "${nids[1]}"x == ""x ];then
        nids[1]=none
    fi
    
    if [ "${nids[0]}"x == "none"x ]&&[ "${nids[1]}"x == "none"x ];then
        echo $origin_nid
        return
    fi
    local ret_1=$(connect_node_do_cmd ${nids[0]} "zfs list -H $nas 2>/dev/null|wc -l")
    local ret_2=$(connect_node_do_cmd ${nids[1]} "zfs list -H $nas 2>/dev/null|wc -l")
    if [ "$ret_1"x == "1"x ];then
        echo ${nids[0]}
        return 
    elif [ "$ret_2"x == "1"x ];then
        echo ${nids[1]}
        return
    fi
    if [ "$ret_1"x == "0"x ]&&[ "$ret_2"x == "0"x ];then
        echo none
        return 
    fi
    echo $origin_nid
    return
}

#去sbb检查nas是否切换 ，若切换返回切换后新链表，未切换返回原列表
function check_duallive_is_switch()
{
    local mnid=$1
    local mnas=$2
    local snid=$3
    local snas=$4
    local mwip=$5
    local mwif=$6
    local netmask=$7
    local msbbid=`get_sbbid_from_nid $mnid`
    local ssbbid=`get_sbbid_from_nid $snid`
    
    local current_mnid=`get_current_nid_form_nas $msbbid $mnid $mnas`
    local current_snid=`get_current_nid_form_nas $ssbbid $snid $snas`
    if [ "$current_mnid"x == "none"x ]||[ "$current_snid"x == "none"x ];then
        echo $mnid $mnas DELETE
        return
    fi
    
    echo $current_mnid $mnas $current_snid $snas $mwip $mwif $netmask UPDATE
}

CM_GETBATCH_TMP_FILE=/tmp/duallive_getbatch_switch.txt

function get_without_repetition_nid()
{
    local src_nids=($1)

    local tmp_arr[0]=${src_nids[0]}
    echo ${src_nids[0]}

    local index=0;
    for nid in ${src_nids[@]}
    do
        local flag=0
        for ((i=0;$i<=$index;++i))
        do
            if [ "$nid"x == "${tmp_arr[$i]}"x ];then
                ((flag++))
            fi
        done
        ((index++))
        if [ $flag -eq 0 ];then
            tmp_arr[$index]=$nid
            echo $nid
        fi
    done
}

function cm_cnm_update_duallive_list()
{
    if [ $((`cat $CM_CNM_DUALLIVE_INI|grep -w CM_CNM_MASTER|wc -l`)) -eq 0 ];then
        return $CM_OK
    fi
    cat $CM_CNM_DUALLIVE_INI|grep -w CM_CNM_MASTER > $CM_GETBATCH_TMP_FILE
    while read line
    do
        local daullive_list=($line)
        local mnid=${daullive_list[0]}
        local snid=${daullive_list[2]}
        local mbknid=`get_brother_nid $mnid`
        local sbknid=`get_brother_nid $snid`
        local nids=$(get_without_repetition_nid "$mnid $snid $mbknid $sbknid")
        local ret=`check_duallive_is_switch $line`
        local flag=${ret##* }
        local info=${ret% *}
        if [ "$flag"x == "DELETE"x ];then
            local ret_str=`connect_node_do_script "$nids" do_delete_ini "$info" $CM_OK`
        elif [ "$flag"x == "UPDATE"x ];then
            if [[ $line =~ $info ]];then
                return
            fi
            local ret_str=$(connect_node_do_script "$nids" do_delete_ini "$mnid $mnas" $CM_OK)
            sleep 0.05
            ret_str=$(connect_node_do_script "$nids" do_record_ini "$info" $CM_OK)
        fi
    done < $CM_GETBATCH_TMP_FILE
    rm -f $CM_GETBATCH_TMP_FILE
}

#getbatch 在每个节点找该节点角色为主的备份列表 nidongde/fs1 18488 poola/fs1 18601 off 1
function cm_cnm_duallive_getbatch()
{
    cm_cnm_update_duallive_list #更新duallive列表
    sleep 0.1
    if [ $((`cat $CM_CNM_DUALLIVE_INI|grep -w CM_CNM_MASTER|wc -l`)) -eq 0 ];then
        return $CM_OK
    fi
    cat $CM_CNM_DUALLIVE_INI|grep -w CM_CNM_MASTER > $CM_GETBATCH_TMP_FILE
    while read line
    do
        local duallive_list=($line)
        local mnid=${duallive_list[0]}
        local mnas=${duallive_list[1]}
        local snid=${duallive_list[2]}
        local snas=${duallive_list[3]}
        local wkip=${duallive_list[4]}
        local wkif=${duallive_list[5]}
        ret=($(zfs get -H syncstatus,bNassync $mnas|awk '{print $3}'))
        local status=${ret[0]}
        local nassync=${ret[1]}
        local nassyncnum=0
        if [ "$nassync"x == "async"x ];then
            nassyncnum=1
        elif [ "$nassync"x == "sync"x ];then
            nassyncnum=2
        else
            nassyncnum=0
        fi
        echo $mnas $mnid $snas $snid $status $nassyncnum $wkif $wkip
    done < $CM_GETBATCH_TMP_FILE
    rm -f $CM_GETBATCH_TMP_FILE
}

function cm_cnm_duallive_count()
{
    cm_cnm_update_duallive_list #更新duallive列表
    sleep 0.1
    cat $CM_CNM_DUALLIVE_INI|grep -w CM_CNM_MASTER|wc -l
    return $CM_OK
}

#判断总结
function cm_cnm_check_err_and_report()
{
    local err_info=$1
    local iRet=$CM_OK
    for err in $err_info
    do
        if [ $err -ne $CM_OK ];then
            iRet=$err
            break
        fi
    done
    return $iRet 
}

cm_cnm_synctypes=("off" "async" "sync")
#创建备份指令1、下发函数 2、接受错误码上报 3、成功则配置cm_cnm_duallive.ini
function cm_cnm_duallive_create_connect()
{
    local paranum=$#
    if [ $paranum -lt 8 ];then
        echo '<create> <mind> <mnas> <snid> <snas> <sync> <workip> <workif> <netmask>'
        return $CM_PARAM_ERR
    fi

##########argvs###########
    local mnid=$1
    local mnas=$2
    local snid=$3
    local snas=$4
    local sync=${cm_cnm_synctypes[$5]}
    local mwip=$6
    local mwif=$7
    local netmask=$8
#########################
    
    local masknum=`get_masknum_form_netmask $netmask`
    if [ $? -ne $CM_OK ];then
        CM_LOG "[${FUNCNAME}:${LINENO}] netmask err [$netmask]"
        return $CM_PARAM_ERR
    fi
    cm_cnm_check_ipaddr $mwip
    if [ $?x = ${CM_PARAM_ERR}x ];then
        CM_LOG "[${FUNCNAME}:${LINENO}] err work ip [$mwip]"
        return $CM_PARAM_ERR
    fi
    cm_cnm_is_allow_create $mnid $snid $mnas
    local iRet=$?
    if [ $iRet -ne $CM_OK ];then
        return $iRet
    fi
    local mbknid=`get_brother_nid $mnid`
    local sbknid=`get_brother_nid $snid`
    
    #echo $mnid $snid $mbknid $sbknid
    #下发命令
    local ret=`connect_node_do_script "$mnid $snid $mbknid $sbknid" do_create "$mnid $mnas $snid $snas $sync $mwip $mwif $masknum" $CM_OK`
    echo ret:$ret
    #检查错误序列报错 cm_cnm_check_err_and_report()
    cm_cnm_check_err_and_report "$ret"
    local iRet=$?
    if [ $iRet -ne $CM_OK ];then
        return $iRet
    fi
    if [ "$(ifconfig -a | grep $mwip)"x == ""x ];then
        ifconfig $mwif addif $mwip up
        CM_LOG "[${FUNCNAME}:${LINENO}]add $mwif $mwip"
    fi
    connect_node_do_script "$mnid $snid $mbknid $sbknid" do_record_ini "$mnid $mnas $snid $snas $mwip $mwif $masknum"
    return $CM_OK
}

#备份删除指令下发
function cm_cnm_duallive_delete_connect()
{
    local paranum=$#
    if [ $paranum -lt 2 ];then
        echo '<delete> <mind> <mnas>'
        return $CM_PARAM_ERR
    fi
    local duallive_info=$*
    local mnid=$1
    local mnas=$2
    #拿到备份nas的节点和名称
    local snas_info=(`cat $CM_CNM_DUALLIVE_INI | grep ^"$mnid $mnas" | awk '{print $3 " " $4}'`)
    if [ ${#snas_info[@]} -ne 2 ];then
        return $CM_PARAM_ERR
    fi
    local snid=${snas_info[0]}
    local snas=${snas_info[1]}
    local mbknid=`get_brother_nid $mnid`
    local sbknid=`get_brother_nid $snid`
    local nids=$(get_without_repetition_nid "$mnid $snid $mbknid $sbknid")
    
    local ifinfo=(`cat $CM_CNM_DUALLIVE_INI | grep ^"$mnid $mnas" | awk '{print $6 " " $5}'`)
    #下发命令
    local ret=`connect_node_do_script "$nids" do_delete "$duallive_info $snid $snas" $CM_OK`

    echo ret:$ret
    #检查错误序列报错 cm_cnm_check_err_and_report()
    cm_cnm_check_err_and_report "$ret"
    local iRet=$?
    if [ $iRet -ne $CM_OK ];then
        return $iRet
    fi
    
    #删除业务ip
    if [[ "$(ifconfig -a | grep ${ifinfo[1]})" ]];then
        ifconfig ${ifinfo[0]} removeif ${ifinfo[1]}
        CM_LOG "[${FUNCNAME}:${LINENO}]delete ${ifinfo[0]} ${ifinfo[1]}"
    fi
    connect_node_do_script "$mnid $snid $mbknid $sbknid" do_delete_ini "$duallive_info"
    return $CM_OK
}


#确定操作、下发集体执行、接受回复、上报结果
function cm_cnm_duallive_main()
{
    local cmd=$1
    local nas=$2    #用户指定的主从nas：描述方式（节点id + nas名）
    local argv=$3   #创建时所需的参数：如同步模式#<argv: 同步模式 业务IP：业务网口：业务掩码>
    local iRet=$CM_OK
    case $cmd in
        create)
            cm_cnm_duallive_create_connect $nas $argv
            iRet=$?
        ;;
        getbatch)
            cm_cnm_duallive_getbatch
            iRet=$?
        ;;
        delete)
            cm_cnm_duallive_delete_connect $nas
            iRet=$?
        ;;
        count)
            cm_cnm_duallive_count
            iRet=$?
        ;;

        do_create)
            cm_cnm_duallive_create $nas $argv
            iRet=$?
        ;;
        do_delete)
            cm_cnm_duallive_delete $nas
            iRet=$?
        ;;
#    添加双活记录    #
        do_record_ini)
            cm_cnm_duallive_record_ini $nas $argv
            iRet=$?
        ;;
#    删除双活记录    #
        do_delete_ini)
            cm_cnm_duallive_delete_ini $nas $argv
            iRet=$?
        ;;
    esac
    return $iRet
}
#*                 $1:      cmd*#
#*      "$2 $3 $4 $5": nas info*#
#*"$6 $7 $8 $9 ${10}":    argvs*#
cm_cnm_duallive_main $1 "$2 $3 $4 $5" "$6 $7 $8 $9 ${10}" #cmd nas argv
exit $?
