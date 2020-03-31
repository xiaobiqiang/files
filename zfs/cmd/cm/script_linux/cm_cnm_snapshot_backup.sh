#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

CM_LOG_ENABLE=1

CM_SSHD_CFG_FILE="/etc/ssh/sshd_config"
CM_USER_ATTR_FILE="/etc/user_attr"
CM_SSH_KEY_DIR="/etc/root/.ssh"
CM_ETC_HOSTS="/etc/inet/hosts"

function cm_cnm_snapshot_backup_init_sshd()
{
    local tmpfile=${CM_SSHD_CFG_FILE}.tmp
    cp ${CM_SSHD_CFG_FILE} "${CM_SSHD_CFG_FILE}.backup"
    rm $tmpfile 2>/dev/null
    cat ${CM_SSHD_CFG_FILE} | awk '{if($1=="PermitEmptyPasswords" || $1=="PermitRootLogin"){$2="yes";print;} else{print;};}' > $tmpfile
    mv $tmpfile ${CM_SSHD_CFG_FILE}
    return $CM_OK
}

function cm_cnm_snapshot_backup_term_sshd()
{
    local tmpfile=${CM_SSHD_CFG_FILE}.tmp
    local bkupfile=${CM_SSHD_CFG_FILE}.backup

    if [ -f $bkupfile ]; then
        mv ${bkupfile} ${CM_SSHD_CFG_FILE}
    else
        rm $tmpfile 2>/dev/null
        cat ${CM_SSHD_CFG_FILE} | awk '{if($1=="PermitEmptyPasswords" || $1=="PermitRootLogin"){$2="no";print;} else{print;};}' > $tmpfile
        mv $tmpfile ${CM_SSHD_CFG_FILE}
    fi
    return $CM_OK
}

function cm_cnm_snapshot_backup_init_mtu()
{
    local mport=`cm_get_localmanageport`
    ifconfig $mport mtu 1500
    return $CM_OK
}

function cm_cnm_snapshot_backup_init_user_attr()
{
    local tmpfile=${CM_USER_ATTR_FILE}.tmp
    local bkupfile=${CM_USER_ATTR_FILE}.backup
    
    if [ ! -f $bkupfile ]; then
        cp ${CM_USER_ATTR_FILE} ${bkupfile}
    fi
    rm $tmpfile 2>/dev/null
        cat ${CM_USER_ATTR_FILE} | awk 'BEGIN{FS=OFS=";"} {if($1~/root/){$6="type=normal";} print}' > $tmpfile
    mv ${tmpfile} ${CM_USER_ATTR_FILE}
    return $CM_OK
}

function cm_cnm_snapshot_backup_term_user_attr()
{
    local tmpfile=${CM_USER_ATTR_FILE}.tmp
    
    local bkupfile=${CM_USER_ATTR_FILE}.backup
    
    if [ -f ${bkupfile} ]; then
        mv ${bkupfile} ${CM_USER_ATTR_FILE}
        return $CM_OK
    fi

    rm $tmpfile

    #illumos不支持
    local ostype=`cm_os_type_get`
    if [ $ostype -eq ${CM_OS_TYPE_SOLARIS} ];then
        cat ${CM_USER_ATTR_FILE} | awk 'BEGIN{FS=OFS=";"} {if($1~/root/){$6="type=role";} print}' > $tmpfile
    elif [ $ostype -eq ${CM_OS_TYPE_ILLUMOS} ];then
        cat ${CM_USER_ATTR_FILE} | awk 'BEGIN{FS=OFS=";"} {if($1~/root/){$6="";} print}' > $tmpfile
    fi
    
    mv ${tmpfile} ${CM_USER_ATTR_FILE}
    return $CM_OK
}

function cm_cnm_snapshot_backup_init_ssh_key()
{
    mkdir -p $CM_SSH_KEY_DIR
    rm $CM_SSH_KEY_DIR/* >> /dev/null
    #嵌套expect 脚本
/usr/bin/expect<<-EOF
set timeout 5
spawn ssh-keygen -t rsa
expect "*:"
send "\r"
expect "*:"
send "\r"
expect "*:"
send "\r"
interact
expect eof
EOF
    return $CM_OK
}

function cm_cnm_snapshot_backup_term_ssh_key()
{
    rm $CM_SSH_KEY_DIR"/"*
    return $CM_OK
}

# param: <local_ip> <remote_ip>
function cm_cnm_snapshot_backup_init_hosts()
{
	local lname=$(hostname)
	local rname=$(timeout 1 ceres_exec $2 'hostname')
	[ -z "$rname" ]&&return $CM_FAIL
	echo "$1 $lname loghost" >> ${CM_ETC_HOSTS}
	echo "$2 $rname loghost" >> ${CM_ETC_HOSTS}
	return $CM_OK
}

function cm_cnm_snapshot_backup_term_hosts()
{
	local local_ip=$1
	local remote_ip=$2
	local tmpfile="/etc/hosts.tmp"
	sed "/$local_ip/d" ${CM_ETC_HOSTS} > $tmpfile
	sed "/$remote_ip/d" $tmpfile > ${CM_ETC_HOSTS}
	mv $tmpfile
	return $CM_OK
}

# param: vol_path
function cm_cnm_snapshot_backup_delete_lu()
{
    local dstdir=$1
    local dst_lu=$(stmfadm list-all-luns | awk "{if(\$2 == \"${dstdir}\"){print \$1}}")
    if [ ! -z "${dst_lu}" ]; then 
        # 下面两步是判断该LU确实是本节点的LUN的LU,而不是其他节点LUN的LU,list-all-luns会列出集群所有的LU
        local datafile=$(stmfadm list-lu -v $dst_lu | awk '/Data/{print $4}')
        [ $datafile == /dev/zvol/rdsk/$dstdir ]&&stmfadm delete-lu $dst_lu
    fi
    return $?
}

# param: <local_ip> <param> <flag>(local remote)
# cnt: 3
# backup_param:
# 同一节点备份: 0|remote_ip|snapshot_path|dst_dir|nid
# 不同节点备份: 1|remote_ip|snapshot_path|dst_dir|nid
function cm_cnm_snapshot_backup_init()
{
	local local_ip=$1			
	local isLocal=$(echo "$2" | cut -d '|' -f 1)
	local remote_ip=$(echo "$2" | cut -d '|' -f 2)
	# source snapshot path
	local path=$(echo "$2" | cut -d '|' -f 3 | cut -d '@' -f 1)
	# dst snapshot dir
	local dstdir=$(echo "$2" | cut -d '|' -f 4)

	if [ $isLocal -eq 0 ]; then 
        cm_cnm_snapshot_backup_delete_lu $dstdir
        if [ $? -ne 0 ]; then 
            CM_LOG "[${FUNCNAME}:${LINENO}] local backup delete lu fail"
            return $CM_FAIL
        fi
        return $CM_OK
    fi
    
    cm_cnm_snapshot_backup_init_sshd
    cm_cnm_snapshot_backup_init_mtu
    cm_cnm_snapshot_backup_init_user_attr
    cm_cnm_snapshot_backup_init_hosts $local_ip $remote_ip
    cm_cnm_snapshot_backup_init_ssh_key
	
	# 将local_ip覆盖param中的remote_ip
    local rem_param=$(echo "$2" | sed "s/$remote_ip/$local_ip/")
	if [ "$3" == "local" ]; then
		timeout 10 ceres_exec $remote_ip "/var/cm/script/cm_cnm_snapshot_backup.sh init $remote_ip '$rem_param' 'remote'"
		if [ $? -ne 0 ]; then 
			CM_LOG "[${FUNCNAME}:${LINENO}] snapshot backup remote init fail"
			return $CM_FAIL
		fi 
		timeout 2 ceres_exec $remote_ip "cat ${CM_SSH_KEY_DIR}/id_rsa.pub" > ${CM_SSH_KEY_DIR}/authorized_keys
		if [ $? -ne 0 ]; then 
			CM_LOG "[${FUNCNAME}:${LINENO}] append remote pub key to authorized_keys fail"
			return $CM_FAIL
		fi 
		# 重启ssh服务让配置生效
		svcadm restart ssh
/usr/bin/expect<<-EOF
set timeout 5
spawn ssh $remote_ip
expect "yes/no"
send "yes\r"
interact
expect eof
EOF
	else
		timeout 2 ceres_exec $remote_ip "cat ${CM_SSH_KEY_DIR}/id_rsa.pub" > ${CM_SSH_KEY_DIR}/authorized_keys
		if [ $? -ne 0 ]; then 
			CM_LOG "[${FUNCNAME}:${LINENO}] snapshot backup remote init fail"
			return $CM_FAIL
		fi
		svcadm restart ssh
		# dstdir是lun的话需要删除LU，不然会报dataset is busy错误
		cm_cnm_snapshot_backup_delete_lu $dstdir
	fi
    return $CM_OK
}

# 检查是否是LUN快照备份或创建LU
# param: <dst_dir>
function cm_cnm_snapshot_backup_check_is_lun()
{
	local dst_dir=$1
	local cnt=$(zfs list -H -t volume "$dst_dir" | wc -l)
	if [ $cnt -ne 0 ]; then 
        stmfadm create-lu /dev/zvol/rdsk/$dstdir
        zfs set zfs:single_data=1 $dstdir 2>/dev/null
    fi
	return $CM_OK
}

# param: <local_ip> <param> <flag>(local remote)
# cnt: 3
# backup_param:
# 同一节点备份: 0|remote_ip|snapshot_path|dst_dir|nid
# 不同节点备份: 1|remote_ip|snapshot_path|dst_dir|nid
function cm_cnm_snapshot_backup_term()
{
	local local_ip=$1			
	local isLocal=$(echo "$2" | cut -d '|' -f 1)
	local remote_ip=$(echo "$2" | cut -d '|' -f 2)
	# source snapshot path
	local path=$(echo "$2" | cut -d '|' -f 3 | cut -d '@' -f 1)
	# dst snapshot dir
	local dstdir=$(echo "$2" | cut -d '|' -f 4)
	local rem_param=$(echo "$2" | sed "s/$remote_ip/$local_ip/")
	if [ "$3" == "local" ]; then 
		# 同节点快照备份,是LUN快照备份就需要为LUN创建LU
		if [ $isLocal -eq 0 ]; then
			cm_cnm_snapshot_backup_check_is_lun $dstdir
			return $?
		# 不是同一节点快照备份
		else
			timeout 10 ceres_exec $remote_ip "/var/cm/script/cm_cnm_snapshot_backup.sh term $remote_ip '${rem_param}' 'remote'"
		fi
	else
		cm_cnm_snapshot_backup_check_is_lun $dstdir
	fi
	cm_cnm_snapshot_backup_term_sshd
	cm_cnm_snapshot_backup_term_user_attr
	cm_cnm_snapshot_backup_term_hosts $local_ip $remote_ip
	cm_cnm_snapshot_backup_term_ssh_key
	svcadm restart ssh
    return $CM_OK
}

function cm_cnm_snapshot_backup_main()
{
	local action=$1
	case $action in
		init):
			[ $# -lt 4 ]&&return $CM_PARAM_ERR
			# <local_ip> <param> <flag>(local remote)
			cm_cnm_snapshot_backup_init "$2" "$3" "$4"
			return $?
			;;
		term):
			[ $# -lt 4 ]&&return $CM_PARAM_ERR
			# <local_ip> <param> <flag>(local remote)
			cm_cnm_snapshot_backup_term "$2" "$3" "$4" 
			return $?
			;;
		*):
			return $CM_ERR_NOT_SUPPORT
			;;
	esac
}

# =============================== main ===================================
cm_cnm_snapshot_backup_main "$1" "$2" "$3" "$4" "$5"
exit $?