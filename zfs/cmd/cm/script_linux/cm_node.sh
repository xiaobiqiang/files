#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

NODEINFODB=/var/cm/data/cm_node.db
CLUSTERCONF=/var/cm/data/cm_cluster.ini
CLUSTERCONF_DEF=/var/cm/static/comm/cm_cluster.ini
CM_OK=0
CM_FAIL=1
CM_PARAM_ERR=3
CM_ERR_CONN_FAIL=14
CM_ERR_NODE_DB_NO_RECORD=12
CM_ERR_NODE_NO_IP=250
CM_ERR_NODE_NONE_CLUSTER_CONF=252
CM_IPMI_IP='/var/cm/static/ipmi_ip.conf'

function set_nas_rpc_nics
{
    if [ ! -d /etc/fsgroup ];then
        mkdir -p /etc/fsgroup
    fi
    local mport=`cm_get_localmanageport`
    local ipaddr=`cm_get_localmanageip`
    is_ipv4_dot_notation $ipaddr
    [ $? -ne 0 ]&&return 0
    [ -e /etc/fsgroup/rpc_port.conf ]&&return 0
cat > /etc/fsgroup/rpc_port.conf << HSTNAME
rpc_port=${mport};
rpc_addr=${ipaddr};
HSTNAME
    return $?
}

function cm_node_cluster_conf
{
    #集群nas配置rpc
    set_nas_rpc_nics
    local ipaddr=`cm_get_localmanageip`
    [ -z $ipaddr ]&&return $CM_ERR_NODE_NO_IP
    local hostid=$(sqlite3 /var/cm/data/cm_node.db "SELECT idx FROM record_t WHERE ipaddr='${ipaddr}'")
    [ -z $hostid ]&&return $CM_ERR_NODE_DB_NO_RECORD
    
    # cluster_name
    local name=$(cat $CLUSTERCONF | awk '/name/{print $3}' 2>/dev/null)
    [ -z $name ]&&return $CM_ERR_NODE_NONE_CLUSTER_CONF
    echo "/usr/sbin/zfs clustersan enable -n $name" > /var/cm/script/cm_cluster_init.sh
    
    echo "sleep 5" >> /var/cm/script/cm_cluster_init.sh
    # mirror
    local mirror=`expr $hostid + 1`
    local mod=`expr $hostid % 2`
    [ $mod -eq 0 ]&&mirror=`expr $hostid - 1`
    echo "/usr/sbin/zfs mirror -e ${mirror}" >> /var/cm/script/cm_cluster_init.sh
    
    # nic(ixgbe0,ixgbe1,...)
    local nics="$(cat $CLUSTERCONF | awk '/nic/{for(i=3;i<=NF;i++)print $i}' | sed 's/,/ /g')"
    local nic=()
    local i=0
    for name in $nics
	do
        nic[i]=$name
        ((i=$i+1))        
	done
    [ -z $nic ]&&return $CM_ERR_NODE_NONE_CLUSTER_CONF
    local priority="$(cat $CLUSTERCONF | awk '/priority/{print $3}')"
#    [ -z $priority ]&&return $CM_ERR_NODE_NONE_CLUSTER_CONF
    local cnt=0
    while [[ ! -z ${nic[$cnt]} ]]; do
        local tmp=`expr $cnt + 1`
        local pri=$(echo $priority | cut -d ',' -f $tmp)
        local opt_pri=
        [ X"${pri}" != X"" ]&&opt_pri="-p ${pri}"
        echo "/usr/sbin/zfs clustersan enable -l ${nic[$cnt]} ${opt_pri}" >> /var/cm/script/cm_cluster_init.sh
        cnt=`expr $cnt + 1`
    done
    
    # ipmi(ON/OFF)
    local ipmi_plumb=$(cat $CLUSTERCONF | awk '/ipmi/{print $3}')
    [ -z $ipmi_plumb ]&&return $CM_ERR_NODE_NONE_CLUSTER_CONF
    ipmi_plumb=$(echo $ipmi_plumb | tr '[:upper:]' '[:lower:]')
    echo "/usr/sbin/zfs clustersan set ipmi=${ipmi_plumb}" >> /var/cm/script/cm_cluster_init.sh
    
    # failover(loadbalance,roundrobin)
    local failover=$(cat $CLUSTERCONF | awk '/failover/{print $3}')
    [ -z $failover ]&&return $CM_ERR_NODE_NONE_CLUSTER_CONF
    echo "/usr/sbin/zfs clustersan set failover=${failover}" >> /var/cm/script/cm_cluster_init.sh
    # 防止重启之后clusternas又关闭了
    echo "zfs multiclus -e" >> /var/cm/script/cm_cluster_init.sh
    
    chmod 755 /usr/sbin/cluster_init.sh
    chmod 755 /var/cm/script/cm_cluster_init.sh
    return $?
}

function is_numeric
{
    typeset value=${1}

    # If "value" is not given, it is not numeric
    if [[ -z "${value}" ]]; then
        return 1
    fi

    # If not numeric, return 1
    if [[ $(expr "${value}" : '[0-9]*') -ne ${#value} ]]; then
        return 1
    fi

    # Numeric
    return 0
}

function is_ipv4_dot_notation
{
    typeset value=${1}
    typeset octets
    typeset octet

#      -A octets $(IFS=. ; set -- ${value};  echo $*)
#    let i=$(set -- ${octets[*]};  echo $#)
    local i=`echo $value|sed 's/\./ /g'|awk '{for(i=1;i<=NF;i++)print $i}'|wc -l`
    octets=`echo $value|sed 's/\./ /g'`   
    # There must be four octets
    if [[ ${i} -ne 4 ]]; then
        return 1
    fi

    # Each must be numeric and less than 256
    for octet in ${octets[*]}
    do
        is_numeric ${octet} || return 1
        if [[ ${octet} -gt 255 ]]; then
            return 1
        fi
    done

    # The first must not be zero
    local first=`echo $octets|awk '{print $1}'`
    if [ $first -eq 0 ];  then
        return 1
    fi

    return 0
}

function cm_node_cluster_init
{
    local lname=$(zfs clustersan state | awk '/name/{printf $3}')
    # 没有加入到集群.
    # 刚装完机也没有
    # 对应节点disable出去，cm崩溃起来
    # disable出去再加进来。
    if [ -z "$lname" ]; then
        local cnt=`sqlite3 /var/cm/data/cm_node.db "SELECT COUNT(id) FROM record_t"`
        # disable出去再加进来
        # 刚装完机加进来
        if [ $cnt -ne 0 ]; then  
            local config_time=$(cat $CLUSTERCONF | awk '/time/{print $3}')
            if [ -z "$config_time" ]; then 
                local myip=`cm_get_localmanageip`
                local rip=$(sqlite3 /var/cm/data/cm_node.db "SELECT ipaddr FROM record_t WHERE ipaddr!='$myip' LIMIT 1")
                [ -z "$rip" ]&&return $CM_FAIL
                timeout 3 ceres_exec $rip "cat $CLUSTERCONF" > $CLUSTERCONF
                timeout 3 ceres_exec $rip "cat /etc/clumgt.config" > /etc/clumgt.config
            fi
            #cm_node_cluster_conf
            #[ $? -ne 0 ]&&return $CM_FAIL
            local state=`service  zfs-clusterd status|grep Active|awk '{print $2}'`
            if [ "X$state" == 'Xinactive' ]; then
                systemctl start zfs-clusterd
            else
                systemctl restart zfs-clusterd
            fi
            sleep 8
            /var/cm/script/cm_cnm.sh node_rdma_add
        fi
        # 首先得有集群SAN才能打开集群NAS
        #zfs multiclus -e
    fi
    
    return 0
}

#==================================================================================
#使用方法
# cm_node.sh init
# cm_node.sh add <nid>
# cm_node.sh delete <nid>
#==================================================================================

#==================================================================================
#                                  初始化
# 输入参数:
#       无
# 回显数据:
#       无
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_node_init
{
    local ip=`cm_get_localmanageip`
    is_ipv4_dot_notation $ip
    if [ $? -ne 0 ]; then   # 若没有ip,直接返回
        return $CM_OK
    fi
    
    # 节点加入集群cm崩溃起来，cm_node_cluster_init不做任何事
    # 节点disable出去，cm崩溃起来，cm_node_cluster_init不做任何事
    # 加节点进来，对面节点执行：
    #                           disable出去再加进来的
    #                           刚装完机加进来的
    cm_node_cluster_init        #对应四种情况。
    
    sqlite3 /var/cm/data/cm_node.db "select name,ipaddr from record_t" |sed 's/|/ /g' >/etc/clumgt.config
    
    #svcadm disable /network/iscsi/target 2>/dev/null
    #svcadm enable /network/iscsi/target 2>/dev/null
    return $CM_OK
}

function cm_node_init_cluster_info
{
    local mport=`cm_get_localmanageport`
    if [ ! -f /var/cm/zfs_clustersan_state.info ]; then
        zfs clustersan state >/var/cm/zfs_clustersan_state.info
    fi
    
    if [ ! -f /var/cm/zfs_clustersan_list_target_v.info ]; then
        zfs clustersan list-target -v >/var/cm/zfs_clustersan_list_target_v.info
    fi
    
    echo '[CLUSTER]' > $CLUSTERCONF
    local val=`md5sum /usr/lib/ceres_cm |awk '{print $1}'`
    echo "id = $val" >> $CLUSTERCONF
    
    #zfs clustersan state
    cat /var/cm/zfs_clustersan_state.info |sed 's/ //g' |egrep "name|failover|ipmi" |sed 's/:/ = /g' >> $CLUSTERCONF
    
    #zfs clustersan list-target -v
    cat /var/cm/zfs_clustersan_list_target_v.info |sed 's/ //g' |egrep 'Target|priority' |awk -F':' 'BEGIN{ni="";pr="";prt=""}$1=="Target"{ni=ni""$2", ";prt=""}$1=="priority"&&prt==""{pr=pr""$2", ";prt=$2}END{print "nic = "ni;print "priority = "pr}' |sed 's/, $//g' >> $CLUSTERCONF
    local dttime=`date '+%Y%m%d%H%M%S'`
    echo "time = $dttime" >> $CLUSTERCONF
    val='zfsonlinux'
    echo "version = $val" >> $CLUSTERCONF
    val=`cm_get_localmanageip`
    echo "ip = $val" >> $CLUSTERCONF
    ifconfig $mport|grep netmask|awk '{print $4}' >> $CLUSTERCONF
    val=`netstat -r |grep ^default |awk '{print $2}'`
    if [ "X$val" == "X" ]; then
        val=`sed -n 1p /etc/hostname.${mport} |awk -F'.' '{print $1"."$2"."$3".1"}'`
    fi
    echo "gateway = $val" >> $CLUSTERCONF
    #rm -f /var/cm/zfs*.info
    local product_flag=`grep 'su' /etc/hostid |sed 's/\"//g'`
    if [ $product_flag = 'sub___r' ]; then
        echo "product = DF3000C" >> $CLUSTERCONF
    elif [ $product_flag = 'sud___r' ]; then
        echo "product = DF5000C" >> $CLUSTERCONF
    else
        echo "product = DF8000C" >> $CLUSTERCONF
    fi
    return $CM_OK
}

function cm_node_init_patch
{
    if [ ! -f $CLUSTERCONF ]; then
        cp ${CLUSTERCONF_DEF} $CLUSTERCONF
    fi
    
    if [ ! -f /var/cm/data/cm_alarm.db ]; then
        cp /var/cm/static/comm/cm_alarm_cfg.db /var/cm/static/comm/cm_alarm.db
    fi
    if [ -f $NODEINFODB ]; then
        return $CM_OK
    fi
    if [ ! -f /var/cm/zfs_clustersan_list_host_i.info ]; then
        return $CM_OK
    fi
    if [ ! -f /var/cm/zfs_clustersan_state.info ]; then
        return $CM_OK
    fi
    if [ ! -f /var/cm/zfs_clustersan_list_target_v.info ]; then
        return $CM_OK
    fi
    touch $NODEINFODB
    local init_sql="CREATE TABLE record_t (id INT,ipaddr VARCHAR(64),name VARCHAR(64),subdomain INT,sbbid INT,ismaster TINYINT,idx INT)"
    sqlite3 $NODEINFODB "${init_sql}"
    local cnt=`sqlite3 $NODEINFODB "select count(*) from record_t"`
    
    # zfs clustersan list-host -i
    cat /var/cm/zfs_clustersan_list_host_i.info |sed 's/ //g' |egrep "^host" |awk -F':' '{printf $2" "}$1=="hostip"{print ""}' |while read line
    do
        #(id INT,ipaddr VARCHAR(64),name VARCHAR(64),subdomain INT,sbbid INT,ismaster TINYINT,idx INT)
        local ipaddr=`echo "$line" |awk '{print $3}'`
        local name=`echo "$line" |awk '{print $1}'`
        local idx=`echo "$line" |awk '{print $2}'`
        local id=`echo "$ipaddr" |awk -F'.' '{print $3*512+$4}'`
        local subdomain=0
        local sbbid=0
        ((sbbid=($idx+1)/2))
        local ismaster=0
        ((ismaster=$idx&1))
        sqlite3 $NODEINFODB "INSERT INTO record_t values($id,'$ipaddr','$name',$subdomain,$sbbid,$ismaster,$idx)"
    done
    
    cm_node_init_cluster_info
    return $CM_OK
}
#==================================================================================
#                                  添加节点
# 输入参数:
#       <nid>
# 回显数据:
#       无
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_node_add
{
    local lip=`cm_get_localmanageip`
    is_ipv4_dot_notation $lip
    [ $? -ne 0 ]&&return $CM_ERR_NODE_NO_IP         # 若没有ip,说明还没配置，直接返回
    
    local cluster=`cat $CLUSTERCONF | awk '/name/{printf $3}' 2>/dev/null`
    [ -z $cluster ]&&return $CM_ERR_NODE_NONE_CLUSTER_CONF      # 还没有配置集群。
    
    local cnt=$(sqlite3 /var/cm/data/cm_node.db "SELECT count(id) FROM record_t")
    [ $cnt -eq 0 ]&&return $CM_ERR_NODE_DB_NO_RECORD        # 没有插入记录到表中，直接返回
    
    local lname=$(zfs clustersan state | awk '/name/{printf $3}')
    local myname=$(cat $CLUSTERCONF | awk '/name/{printf $3}')
    if [ -z ${lname} -o "$lname" != "$myname" ]; then   # 第一个节点第一次加节点,把自己加进去
        local myip=`cm_get_localmanageip`
        local hname=$(sqlite3 /var/cm/data/cm_node.db "SELECT name FROM record_t WHERE ipaddr='${myip}'")
        #echo "#Add cluter node and ip" >/etc/clumgt.config
        #echo $hname $myip >> /etc/clumgt.config
    fi
    
    # 第一次加节点时，节点本身还没有初始化
    # 其他时候不会执行动作
    cm_node_cluster_init
    
    local node=$(sqlite3 /var/cm/data/cm_node.db "SELECT name,ipaddr,idx FROM record_t WHERE id=$1")
    [ -z $node ]&&return $CM_ERR_NODE_DB_NO_RECORD
    
    local rname=$(echo "$node" | awk 'BEGIN{FS="|"} {printf $1}')
    local rip=$(echo "$node" | awk 'BEGIN{FS="|"} {printf $2}')
    local ridx=$(echo "$node" | awk 'BEGIN{FS="|"} {printf $3}')
    local myip=`cm_get_localmanageip`
    
    # 同时向/etc/clumgt.config添加节点信息
    #echo $rname $rip >>/etc/clumgt.config
    sqlite3 /var/cm/data/cm_node.db "select name,ipaddr from record_t" |sed 's/|/ /g' >/etc/clumgt.config
    
    /var/cm/script/cm_cnm.sh node_rdma_add
    return $?
}

#==================================================================================
#                                  删除节点
# 输入参数:
#       <nid>
# 回显数据:
#       无
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_node_delete
{
    local nid=$1
    local rip=$(sqlite3 /var/cm/data/cm_node.db "SELECT ipaddr FROM record_t WHERE id = ${nid}")
    [ -z $rip ]&&return $CM_ERR_NODE_DB_NO_RECORD
    local myip=`cm_get_localmanageip`
    if [ "$rip" == "$myip" ]; then  # 本节点就是删除的节点
        zfs clustersan disable -c -f &
        echo "#Add cluter node and ip" >/etc/clumgt.config  # 清空
        echo "" > $CLUSTERCONF        # 清空
        #清除集群配置信息
        #sed '/\/usr\/sbin\/zfs/d' /var/cm/script/cm_cluster_init.sh > /tmp/cm_cluster_init.sh
        #sed '/.*sleep.*/d' /tmp/cm_cluster_init.sh > /var/cm/script/cm_cluster_init.sh
    else
        # 其他每个节点执行
        sed "/${rip}/d" /etc/clumgt.config >/tmp/clumgt.tmp # 清除这条记录
        mv /tmp/clumgt.tmp /etc/clumgt.config
    fi
    return $?
}

#==================================================================================
#                                  节点(nid)远程上电
# 输入参数:
#       <nid> <username> <password>
# 回显数据:
#       无
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_node_power_on
{
    # 获取远程上电节点的hostid
    local rem_hid=$(sqlite3 /var/cm/data/cm_node.db "SELECT idx FROM record_t WHERE id=$1")
    [ -z $rem_hid ]&&return $CM_FAIL
    # 根据hostid查找到对应的ipmi_ip
    local ipmi_ip=$(cat $CM_IPMI_IP|awk '$2=="'$rem_hid'"{print $1}')
    is_ipv4_dot_notation "$ipmi_ip"
    [ $? -ne 0 ]&&return $CM_FAIL
    ping $ipmi_ip -i 1 -c 1 >/dev/null
    [ $? -ne 0 ]&&return $CM_ERR_CONN_FAIL
    
    local user=$2
    local password=$3
/usr/bin/expect <<EOF
set timeout 2
spawn ipmitool -I lanplus -H $ipmi_ip -U $user power on
expect "d:"
send "${password}\r"
interact
expect eof
catch wait ret
exit [lindex \$ret 3]
EOF
    return $?
}

#==================================================================================
#                                  节点(nid)远程下电
# 输入参数:
#       <nid>
# 回显数据:
#       无
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_node_power_off
{
    # 获取远程下电节点的ip
    local rem_ip=$(sqlite3 /var/cm/data/cm_node.db "SELECT ipaddr FROM record_t WHERE id=$1")
    [ -z $rem_ip ]&&return $CM_ERR_NODE_DB_NO_RECORD
    local myip=`cm_get_localmanageip`
    if [ "${myip}" == "${rem_ip}" ]; then 
        poweroff 2>/dev/null
    else 
        timeout 10 ceres_exec $rem_ip "poweroff"
    fi
    
    return $CM_OK
}

#==================================================================================
#                                  节点(nid)远程重启
# 输入参数:
#       <nid>
# 回显数据:
#       无
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_node_reboot
{
    # 获取远程重启节点的ip
    local rem_ip=$(sqlite3 /var/cm/data/cm_node.db "SELECT ipaddr FROM record_t WHERE id=$1")
    [ -z $rem_ip ]&&return $CM_ERR_NODE_DB_NO_RECORD
    local myip=`cm_get_localmanageip`
    if [ "${myip}" == "${rem_ip}" ]; then 
        reboot 2>/dev/null
    else 
        timeout 10 ceres_exec $rem_ip "reboot"
    fi
    return $CM_OK
}

#==================================================================================
# 主函数
#==================================================================================
function cm_node_main
{
    local paramnum=$#
    local action=$1
    local iRet=$CM_ERR_NOT_SUPPORT
    
    case $action in
        init)
            cm_node_init
            iRet=$?
            ;;
        add) 
            if [ $paramnum != 2 ]; then
                return $CM_PARAM_ERR
            fi
            cm_node_add $2
            iRet=$?
            ;;
        delete)
            if [ $paramnum != 2 ]; then
                return $CM_PARAM_ERR
            fi
            cm_node_delete $2
            iRet=$?
            ;;
        on)
            if [ $paramnum != 4 ]; then
                return $CM_PARAM_ERR
            fi
            cm_node_power_on $2 $3 $4
            iRet=$?
            ;;
        off)
            if [ $paramnum != 2 ]; then
                return $CM_PARAM_ERR
            fi
            cm_node_power_off $2
            iRet=$?
            ;;
        reboot)
            if [ $paramnum != 2 ]; then
                return $CM_PARAM_ERR
            fi
            cm_node_reboot $2
            iRet=$?
            ;;
        preinit)
            cm_node_init_patch
            iRet=$?
            ;;
        cluinit)
            cm_node_init_cluster_info
            iRet=$?
            ;;
        *)
            iRet=$CM_PARAM_ERR
            ;;
    esac
    return $iRet
}
#==================================================================================
cm_node_main $@
exit $?