#!/bin/bash
source '/var/cm/script/cm_types.sh'

begin='/etc/hosts'
dest='/var/cm/script/iptrans.tmp'
#==================================================================================
#                                  修改记录
# 输入参数:
#       <ip>
#       <domain>
#       <new_ip>
#       <new_domain>
# 回显数据:
#       null
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_dns_modify()
{
	local domain=$1
	local new_ip=$2
	cat $begin | sed  "/"$domain"$/d" >$dest
	echo "$new_ip $domain">>$dest
	cat $dest>$begin 
	return $CM_OK
}
#==================================================================================
#                                  添加记录
# 输入参数:
#       <ip>
#       <domain>
# 回显数据:
#       null
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_dns_insert()
{
	local ip=$1
	local domain=$2
    local num
	num=`grep -w "$domain" $begin | wc -l | awk -F' ' '{print $1}'`
	if [ $num -eq 0 ]; then
		echo "$ip $domain" >> $begin
		return $CM_OK
		
	else
	    return $CM_ERR_ALREADY_EXISTS
		
	fi
		
}
#==================================================================================
#                                  删除记录
# 输入参数:
#       <ip>
#       <domain>
# 回显数据:
#       null
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_dns_delete()
{
	local domain=$1
	cat $begin | sed  "/"$domain"$/d" >$dest
	cat $dest>$begin
	return $CM_OK
}
#==================================================================================
#                                  记录查询
# 输入参数:
#       <ip> or
#		<domain>	
# 回显数据:
# 		cut
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_dns_get()
{
	local ip_or_domain_or_both=$1
	cat $begin |grep -w "$ip_or_domain_or_both"
	return $CM_OK
  
}

#==================================================================================
#                                  记录数查询
# 输入参数:
#       NULL	
# 回显数据:
# 		cnt
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_dns_count()
{
	local num
	local ip_line
	local last_line
	cat $begin | grep -v "^$" | grep -v -w 'Prodigy'| grep -v -w 'localhost' >$dest
	num=`cat $begin | egrep -v '#|localhost'|wc -l`
	echo $num
	return $CM_OK
  
}
#==================================================================================
#                                  批量查询
# 输入参数:
#       NULL
# 回显数据:
# ip domain
# 返 回 值:
#       CM_OK
#       ...
#==================================================================================
function cm_cnm_dns_getbatch()
{
	local line_num
	cat $begin | grep -v "^$" | grep -v -w 'Prodigy' | grep -v -w 'localhost'>$dest
	cat $begin | egrep -v '#|localhost'
    return $CM_OK	
}
#==================================================================================
# 主函数
#==================================================================================
function main()
{
	local paramnum=$#
	local iRet=$CM_OK
	case $1 in 
		getbatch)
		if [ $paramnum -ne 1 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_dns_getbatch 
		iRet=$?
		;;
		modify)
		if [ $paramnum -ne 3 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_dns_modify $2 $3  
		iRet=$?
		;;
		delete)
		if [ $paramnum -ne 2 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_dns_delete $2  
		iRet=$?
		;;
		count)
		if [ $paramnum -ne 1 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_dns_count 
		iRet=$?
		;;
		add)
		if [ $paramnum -ne 3 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_dns_insert $2 $3 
		iRet=$?
		;;
		get)
		if [ $paramnum -ne 2 ]; then
            return $CM_PARAM_ERR
        fi
		cm_cnm_dns_get $2  
		iRet=$?
		;;
		*)
        ;;
	esac
	return $iRet
}
main $*
exit $?