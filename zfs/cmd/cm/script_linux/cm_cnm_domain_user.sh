#!/bin/bash
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

typeset -l net
net=`grep 'default_realm' /etc/krb5/krb5.conf|awk '{print $3}'`

function cm_cnm_domain_user_getbatch()
{
    #idmap list | awk '{print $2" "$3}'|sed "s/winuser://g" |sed "s/unixuser://g"|sed "s/@$net//g"
    idmap list |sed 's/[:@]/ /g' |awk '{print $3" "$6}'
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
    local cnt=`idmap list |grep -w "winuser:$domain_user"`
    if [ "X$cnt" != "X" ]; then
        return $CM_ERR_ALREADY_EXISTS
    fi
    
    cnt=`idmap list |grep -w "unixuser:$local_user"`
    if [ "X$cnt" != "X" ]; then
        return $CM_ERR_ALREADY_EXISTS
    fi
    
    if [ `smbadm lookup $domain_user| wc -l` -ne 1 ]; then
        return $CM_ERR_NOT_EXISTS
    fi
    cm_multi_exec "idmap add winuser:$domain_user@$net unixuser:$local_user"
    return $?
}

function cm_cnm_domain_user_delete()
{
    local domain_user=$1
    local local_user=$2
    cm_multi_exec "idmap remove winuser:$domain_user@$net unixuser:$local_user"
    return $?
}

function cm_cnm_domain_user_get_id()
{
	local domain_user=$1
	local local_user=$2
	
	idmap list | grep -n ":$domain_user@" |awk -F':' '{print $1}'
}


cm_cnm_domain_user_$*
exit $?
