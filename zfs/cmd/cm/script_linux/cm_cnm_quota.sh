#!/bin/bash
source '/var/cm/script/cm_types.sh'

#==================================================================================
#                                  ������ѯ
# �������:
#       <usertype>  
#       <filesystem>
# ��������:
# filesystem name space used
# �� �� ֵ:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_quota_getbatch()
{
	local usertype=$1
	local filesystem=$2
	if [ $usertype -eq 0 ]; then
		array_name=($(cat /etc/passwd|awk -F':' '$3>99&&$3<50000{print $1}'))
		len=${#array_name[@]}
		for (( i=0; i<$len; i=i+1 ))
		do
			space=`zfs get userquota@${array_name[i]} $filesystem | awk 'NR>1{printf $3}'`
			softspace=`zfs get softuserquota@${array_name[i]} $filesystem | awk 'NR>1{printf $3}'`
			used=`zfs get userused@${array_name[i]} $filesystem | awk 'NR>1{printf $3}'`
			echo "$filesystem ${array_name[i]} $space $softspace $used"
		done 
	fi
	
	if [ $usertype -eq 1 ]; then
		array_name=($(cat /etc/group|awk -F':' '$3==1||($3>99&&$3<1000){print $1}'))
		len=${#array_name[@]}
		for (( i=0; i<$len; i=i+1 ))
		do
			space=`zfs get groupquota@${array_name[i]} $filesystem | awk 'NR>1{printf $3}'`
			softspace=`zfs get softgroupquota@${array_name[i]} $filesystem | awk 'NR>1{printf $3}'`
			used=`zfs get groupused@${array_name[i]} $filesystem | awk 'NR>1{printf $3}'`
			echo "$filesystem ${array_name[i]} $space $softspace $used"
		done 
	fi
	return $CM_OK
}

#==================================================================================
#                                  �޸ļ�¼
# �������:
#       <usertype>
#       <name>
#       <filesystem>
#       <space>
#		<softspace>
# ��������:
#       null
# �� �� ֵ:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_quota_insert()
{
	local usertype=$1
	local name=$2
	local filesystem=$3
	local space=$4
	local softspace=$5
	
	if [ $space != "null" ]; then
		if [ $usertype -eq 0 ]; then
			zfs set userquota@$name=$space $filesystem
			if [ $? -ne 0 ]; then
				return $CM_FAIL
			fi
		fi
		if [ $usertype -eq 1 ]; then
			zfs set groupquota@$name=$space $filesystem
			if [ $? -ne 0 ]; then
				return $CM_FAIL
			fi
		fi
	fi
	
	if [ $softspace != "null" ]; then
		if [ $usertype -eq 0 ]; then
			zfs set softuserquota@$name=$softspace $filesystem
			if [ $? -ne 0 ]; then
				return $CM_FAIL
			fi
		fi
		if [ $usertype -eq 1 ]; then
			zfs set softgroupquota@$name=$softspace $filesystem
			if [ $? -ne 0 ]; then
				return $CM_FAIL
			fi
		fi
	fi
	return $CM_OK
}

#==================================================================================
#                                  ɾ����¼
# �������:
#       <usertype>
#       <name>
#       <filesystem>
# ��������:
#       null
# �� �� ֵ:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_quota_delete()
{
	local usertype=$1
	local name=$2
	local filesystem=$3
	if [ $usertype -eq 0 ]; then
		zfs set userquota@$name=none $filesystem
		zfs set softuserquota@$name=none $filesystem
	fi
	
	if [ $usertype -eq 1 ]; then
		zfs set groupquota@$name=none $filesystem
		zfs set softgroupquota@$name=none $filesystem
	fi
	return $CM_OK
}

#==================================================================================
#                                  ��¼����ѯ
# �������:
#       <usertype>
#		<filesystem>	
# ��������:
# 		cut
# �� �� ֵ:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_quota_count()
{
	local usertype=$1
	local filesystem=$2
	if [ $usertype -eq 0 ]; then
		cut=`cat /etc/passwd | awk -F':' '($3>99&&$3<50000) {print $3}'| wc -l`
		echo "$cut"
	fi
	
	if [ $usertype -eq 1 ]; then
		cut=`cat /etc/group |awk -F':' '$3==1||($3>99&&$3<1000)' |wc -l`
		echo "$cut"
	fi	
	return $CM_OK
}

#==================================================================================
# ������
#==================================================================================
function main()
{
	local paramnum=$#
	local iRet=$CM_OK
	case $1 in 
		getbatch)
		if [ $paramnum -ne 3 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_quota_getbatch $2 $3
		iRet=$?
		;;
		update)
		if [ $paramnum -ne 6 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_quota_insert $2 $3 $4 $5 $6
		iRet=$?
		;;
		delete)
		if [ $paramnum -ne 4 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_quota_delete $2 $3 $4
		iRet=$?
		;;
		count)
		if [ $paramnum -ne 3 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_quota_count $2 $3
		iRet=$?
		;;
		*)
        ;;
	esac
	return $iRet
}

main $*
exit $?