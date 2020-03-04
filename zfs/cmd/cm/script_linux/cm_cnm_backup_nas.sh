#!/bin/bash

source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

CM_CNM_BACKUP_SH=/var/cm/script/cm_cnm_backup_nas.sh

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

function cm_cnm_backup_cheak_fsid()
{
    local fsid=$1
    if [ "$fsid"x == "0"x ]||[ "$fsid"x == "17261658112"x ]|| [ "$fsid"x == ""x ] || [ "$fsid"x == "-"x ];then #if [ "$fsid"x == "0"x ];then
        CM_LOG "[${FUNCNAME}:${LINENO}]fsid=$fsid err"
        return $CM_PARAM_ERR
    fi
    return $CM_OK
}

#具体的属性设置 参数：属性值 nas
function STATIC_cm_cnm_backup_setattr()
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
function STATIC_cm_cnm_backup_restart()
{
    local nas=$1
    local nas_info=(`zfs list -H -o sharenfs,sharesmb $nas 2>/dev/null`)
    
    zfs umount -f $nas
    if [ $? != $CM_OK ];then
        CM_LOG "[${FUNCNAME}:${LINENO}]zfs umount $nas fail"
        return $CM_FAIL
    fi
    zfs mount $nas
    if [ $? != $CM_OK ];then
        CM_LOG "[${FUNCNAME}:${LINENO}]zfs umount $nas fail"
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
    STATIC_cm_cnm_backup_setattr "mirror $desnas $deshid $synctp $nastype $rfsid" $srcnas #设置nas文件属性
    if [ $? -ne $CM_OK ];then
        CM_LOG "[${FUNCNAME}:${LINENO}]set node:[$(hostname)] fail"
        return $CM_PARAM_ERR
    fi
    
    #重启
    STATIC_cm_cnm_backup_restart $srcnas
    if [ $? -ne $CM_OK ];then
        return $CM_ERR_BUSY
    fi
    return $CM_OK
}

CM_CNM_BACKUP_INI=/etc/cm_cnm_backup.ini

#记录备份关系
#节点列表的格式：主节点：主nas：从节点：从nas：同步方式：同步状态：本节点角色：列表关系(backup/duallive)
function cm_cnm_backup_record_ini()
{
    local backup_info=$*
    local mnid=$1
    local snid=$3
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
        *)
            return $CM_OK #错误的目的IP
        ;;
    esac
    echo "$backup_info $role" >> $CM_CNM_BACKUP_INI
    return $OM_OK
}
#删除备份记录
function cm_cnm_backup_delete_ini()
{
    local TMP_FILE=/tmp/cm_cnm_backup.tmp
    local mnid=$1
    local mnas=$2
    cat /dev/null > $TMP_FILE
    while read line
    do
        if [[ $line =~ ^"$mnid $mnas" ]];then
            continue
        fi
        echo $line
        echo $line >> $TMP_FILE
    done < $CM_CNM_BACKUP_INI
    cat $TMP_FILE > $CM_CNM_BACKUP_INI
    rm -f $TMP_FILE
}


#检测是否nas已经用于其他备份
function cm_cnm_is_allow_create()
{
    local mnas="$1 $2"
    local snas="$3 $4"
    if [ ! -e $CM_CNM_BACKUP_INI ];then
        return $CM_OK
    fi
    #lcoal rezfs get -H remotefsname $srcnas |awk '{print $3}'
    local ini_line1=`cat $CM_CNM_BACKUP_INI|grep -w "$mnas"|wc -l`
    local ini_line2=`cat $CM_CNM_BACKUP_INI|grep -w "$snas"|wc -l`
    local ini_line=$(($ini_line1 + $ini_line2))
    if [ $ini_line -ne 0 ];then
        return $CM_ERR_BUSY
    fi
}

############################################################
#                          create                          #
############################################################
#cm_cnm_backup.ini 记录备份的创建记录（备份列表）

#master 创建备份 1、检查是否允许被创建 2、设置属性 3、检查错误上报
#对底层需求：双活nas需要被保护，不能通过命令任意修改
function master_create_operation()
{
    #info <主节点：主nas：从节点：从nas：同步方式>
    local backup_info=$*
    local mnid=$1
    local mnas=$2
    local snid=$3
    local snas=$4
    local sync=$5
    local shid=`get_hid_from_nid $snid`
    
    cm_cnm_is_allow_create $backup_info
    if [ $? -ne $CM_OK ];then
        return $CM_ERR_BUSY
    fi
    cm_cnm_bankup_setcfg master $mnas $shid $snas $sync 0
    local iRet=$?
    if [ $iRet -ne $CM_OK ];then
        return $iRet
    fi

    return $CM_OK
}

function slave_create_operation()
{
    local backup_info=$*
    local mnid=$1
    local mnas=$2
    local snid=$3
    local snas=$4
    local synctype=$5
    local mhid=`get_hid_from_nid $mnid`
    
    cm_cnm_is_allow_create $backup_info
    if [ $? -ne $CM_OK ];then
        return $CM_ERR_BUSY
    fi
    cm_cnm_bankup_setcfg slave $snas $mhid $mnas $synctype 0
    local iRet=$?
    if [ $iRet -ne $CM_OK ];then
        return $iRet
    fi

    return $CM_OK
}

function cm_cnm_backup_create()
{
    local backup_info=$*
    local mnid=$1
    local mnas=$2
    local snid=$3
    local snas=$4
    local synctype=$5
    local hostid=`cm_get_localid`
    local nid=`get_nid_from_hid $hostid`
    
    case $nid in
        $mnid)
            master_create_operation $backup_info
            return $?
        ;;
        $snid)
            slave_create_operation $backup_info
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
    local have_nas=`cat $FS_XML|grep "type=\"filesystem\" name=\"$nas\""|wc -l`
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

function cm_cnm_backup_delete()
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
    echo $ret_1 $ret_2 > mytest
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
function check_backup_is_switch()
{
    local mnid=$1
    local mnas=$2
    local snid=$3
    local snas=$4
    local msbbid=`get_sbbid_from_nid $mnid`
    local ssbbid=`get_sbbid_from_nid $snid`
    
    local current_mnid=`get_current_nid_form_nas $msbbid $mnid $mnas`
    local current_snid=`get_current_nid_form_nas $ssbbid $snid $snas`
    if [ "$current_mnid"x == "none"x ]||[ "$current_snid"x == "none"x ];then
        echo $mnid $mnas DELETE
        return
    fi
    
    echo $current_mnid $mnas $current_snid $snas UPDATE
}

CM_GETBATCH_TMP_FILE=/tmp/backup_getbatch_switch.txt

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

function cm_cnm_update_backup_list()
{
    if [ $((`cat $CM_CNM_BACKUP_INI|grep -w CM_CNM_MASTER|wc -l`)) -eq 0 ];then
        return $CM_OK
    fi
    cat $CM_CNM_BACKUP_INI|grep -w CM_CNM_MASTER > $CM_GETBATCH_TMP_FILE
    while read line
    do
        local backup_list=($line)
        local mnid=${backup_list[0]}
        local snid=${backup_list[2]}
        local mbknid=`get_brother_nid $mnid`
        local sbknid=`get_brother_nid $snid`
        local nids=$(get_without_repetition_nid "$mnid $snid $mbknid $sbknid")
        local ret=`check_backup_is_switch $line`
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
function cm_cnm_backup_getbatch()
{
    cm_cnm_update_backup_list #更新backup列表
    sleep 0.1
    if [ $((`cat $CM_CNM_BACKUP_INI|grep -w CM_CNM_MASTER|wc -l`)) -eq 0 ];then
        return $CM_OK
    fi
    cat $CM_CNM_BACKUP_INI|grep -w CM_CNM_MASTER > $CM_GETBATCH_TMP_FILE
    while read line
    do
        local backup_list=($line)
        local mnid=${backup_list[0]}
        local mnas=${backup_list[1]}
        local snid=${backup_list[2]}
        local snas=${backup_list[3]}
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
        echo $mnas $mnid $snas $snid $status $nassyncnum
    done < $CM_GETBATCH_TMP_FILE
    rm -f $CM_GETBATCH_TMP_FILE
}

function cm_cnm_backup_count()
{
    cm_cnm_update_backup_list #更新backup列表
    sleep 0.1
    cat $CM_CNM_BACKUP_INI|grep -w CM_CNM_MASTER|wc -l
    return $CM_OK
}

CM_CNM_DELEY_S=600 #定义默认延时

#批量转发函数，返回每个节点的错误码，可以设定超时时间(可选，默认3s)
function connect_node_do_script()
{
    local nodes=($1)
    local cmd="$2"
    local argv="$3"
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
        #echo "ceres_exec $ip \"timeout $deley_s $CM_CNM_BACKUP_SH $cmd $argv\"" >> test
        timeout $deley_s ceres_exec $ip "$CM_CNM_BACKUP_SH $cmd $argv"& #超时返回124 CM_CNM_BACKUP_SH全局定义的脚本名
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
    echo "timeout $deley_s ceres_exec $ip \"$cmd\"" >> test
    local ret_str=`timeout $deley_s ceres_exec $ip "$cmd"` #超时返回124 CM_CNM_BACKUP_SH全局定义的脚本名
    echo $ret_str
    return
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
#创建备份指令1、下发函数 2、接受错误码上报 3、成功则配置cm_cnm_backup.ini
function cm_cnm_backup_create_connect()
{
    local paranum=$#
    if [ $paranum -lt 5 ];then
        echo '<create> <mind> <mnas> <snid> <snas> <sync>'
        return $CM_PARAM_ERR
    fi
    local mnid=$1
    local mnas=$2
    local snid=$3
    local snas=$4
    local sync=${cm_cnm_synctypes[$5]}
    
    #下发命令
    local ret=`connect_node_do_script "$mnid $snid $mbknid $sbknid" do_create "$mnid $mnas $snid $snas $sync" $CM_OK`
    echo ret:$ret
    #检查错误序列报错 cm_cnm_check_err_and_report()
    cm_cnm_check_err_and_report "$ret"
    local iRet=$?
    if [ $iRet -ne $CM_OK ];then
        return $iRet
    fi
    if [ "$mnid"x == "$snid"x ];then
        connect_node_do_script "$mnid" do_record_ini "$mnid $mnas $snid $snas $sync"
    else
        connect_node_do_script "$mnid $snid" do_record_ini "$mnid $mnas $snid $snas $sync"
    fi
    return $CM_OK
}

#备份删除指令下发
function cm_cnm_backup_delete_connect()
{
    local paranum=$#
    if [ $paranum -lt 2 ];then
        echo '<delete> <mind> <mnas>'
        return $CM_PARAM_ERR
    fi
    local backup_info=$*
    local mnid=$1
    local mnas=$2
    #拿到备份nas的节点和名称
    local snas_info=(`cat $CM_CNM_BACKUP_INI | grep ^"$mnid $mnas" | awk '{print $3 " " $4}'`)
    if [ ${#snas_info[@]} -ne 2 ];then
        return $CM_PARAM_ERR
    fi
    local snid=${snas_info[0]}
    local snas=${snas_info[1]}
    local mbknid=`get_brother_nid $mnid`
    local sbknid=`get_brother_nid $snid`

    #下发命令
    local ret=$(connect_node_do_script "$mnid $snid $mbknid $sbknid" do_delete "$backup_info $snid $snas" $CM_OK)

    echo ret:$ret
    #检查错误序列报错 cm_cnm_check_err_and_report()
    cm_cnm_check_err_and_report "$ret"
    local iRet=$?
    if [ $iRet -ne $CM_OK ];then
        return $iRet
    fi
    connect_node_do_script "$mnid $snid" do_delete_ini "$backup_info"
    return $CM_OK
}


#确定操作、下发集体执行、接受回复、上报结果
function cm_cnm_backup_main()
{
    local cmd=$1
    local nas=$2    #用户指定的主从nas：描述方式（节点id + nas名）
    local argv=$3   #创建时所需的参数：如同步模式
    local iRet=$CM_OK
    case $cmd in
        create)
            cm_cnm_backup_create_connect $nas $argv
            iRet=$?
        ;;
        getbatch)
            cm_cnm_backup_getbatch
            iRet=$?
        ;;
        delete)
            cm_cnm_backup_delete_connect $nas $argv
            iRet=$?
        ;;
        count)
            cm_cnm_backup_count
            iRet=$?
        ;;

        do_create)
            cm_cnm_backup_create $nas $argv
            iRet=$?
        ;;
        do_delete)
            cm_cnm_backup_delete $nas
            iRet=$?
        ;;
        do_record_ini)
            cm_cnm_backup_record_ini $nas $argv
            iRet=$?
        ;;
        do_delete_ini)
            cm_cnm_backup_delete_ini $nas $argv
            iRet=$?
        ;;
    esac
    return $iRet
}

cm_cnm_backup_main $1 "$2 $3 $4 $5" "$6 $7 $8 $9 ${10}" #cmd nas argv
exit $?