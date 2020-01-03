#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

typeset -l net
net=`grep 'default_realm' /etc/krb5/krb5.conf|awk '{print $3}'`

function cm_cnm_domain_user_getbatch()
{
	idmap list | awk '{print $2" "$3}'|sed "s/winuser://g" |sed "s/unixuser://g"|sed "s/@$net//g"
	return $CM_OK
}

function cm_cnm_domain_user_count()
{
	idmap list | wc -l
	return $CM_OK
}

function cm_cnm_domain_user_insert()
{
	local domain_user=$1
	local local_user=$2
	local iRet=$CM_OK
	
	if [ `smbadm lookup $domain_user| wc -l` -ne 1 ]; then
		return $CM_PARAM_ERR
	fi
	
	idmap add winuser:$domain_user@$net unixuser:$local_user
	iRet=$?
	if [ $iRet -ne $CM_OK ]; then
		return $CM_FAIL
	fi
	return $CM_OK
}

function cm_cnm_domain_user_delete()
{
	local id=$1
	local domain_user
	local local_user
	local iRet=$CM_OK

	domain_user=`idmap list|awk 'NR=='$id'{print}'|awk '{print $2}'`
	local_user=`idmap list|awk 'NR=='$id'{print}'|awk '{print $3}'`
	
	idmap remove $domain_user $local_user
	iRet=$?
	if [ $iRet -ne $CM_OK ]; then
		return $CM_FAIL
	fi
	return $CM_OK
}

function cm_cnm_domain_user_get_id()
{
	local domain_user=$1
	local local_user=$2
	
	idmap list | grep -n ":$domain_user@" |awk -F':' '{print $1}'
}


cm_cnm_domain_user_$*
exit $?
