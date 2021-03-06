#!/bin/bash
cfgfile='/etc/ntp.conf'
source '/var/cm/script/cm_types.sh'
source '/var/cm/script/cm_common.sh'

OS_TYPE=`cm_systerm_version_get`

function cm_cnm_ntp_server()
{
    if [ `tail -n 1 $cfgfile |grep server |wc -l` -ne 0 ]; then
        sed -i '$c server 127.127.1.0' $cfgfile
    else
        echo 'server 127.127.1.0' >> $cfgfile
    fi
    if [ $OS_TYPE -eq $CM_OS_TYPE_DEEPIN ];then
        systemctl restart ntp
    else
        systemctl restart ntpd
    fi
    return $?
}

function cm_cnm_ntp_client()
{
    while [ 1 ]
    do
        ip=`ceres_cmd master| grep node_ip|awk '{print $3}'`
        if [ "X" = "X"$ip ]; then
            sleep 1
        else
            break
        fi
    done
    if [ `tail -n 1 $cfgfile |grep server |wc -l` -ne 0 ]; then
        sed -i '$c server '$ip'' $cfgfile
    else
        echo "server $ip" >> $cfgfile
    fi
    if [ $OS_TYPE -eq $CM_OS_TYPE_DEEPIN ];then
        systemctl restart ntp
    else
        systemctl restart ntpd
    fi
    return $?
}

function cm_cnm_ntp_start()
{

	master=`ceres_cmd master| grep node_ip|awk '{print $3}'`
	cluster_ip=`ceres_cmd node |grep '\.'|awk '{print $4}'`
	
	cm_cnm_ntp_server

	for node in $cluster_ip
	do
		if [ "$master" == "$node" ]; then
			continue
		fi
		ceres_exec $node "/var/cm/script/cm_cnm_ntp.sh client"
	done
}

function cm_cnm_ntp_close()
{
    if [ `tail -n 1 $cfgfile |grep server |wc -l` -ne 0 ]; then
        sed -i '$d' $cfgfile
    fi
    
    if [ $OS_TYPE -eq $CM_OS_TYPE_DEEPIN ];then
        systemctl stop ntp
    else
        systemctl stop ntpd
    fi
    return $?
}

case $1 in 
	server)
	cm_cnm_ntp_start 
	;;
	client)
	cm_cnm_ntp_client 
	;;
	close)
	cm_cnm_ntp_close
	;;
	*)
	;;
esac

exit



