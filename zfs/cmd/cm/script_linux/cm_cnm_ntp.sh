#!/bin/bash
cfgfile='/etc/ntp.conf'

function cm_cnm_ntp_server()
{
    if [ `tail -n 1 $cfgfile |grep server |wc -l` -ne 0 ]; then
        sed -i '$c server 127.127.1.0' $cfgfile
    else
        echo 'server 127.127.1.0' >> $cfgfile
    fi
    systemctl restart ntpd
}

function cm_cnm_ntp_client()
{
    ip=`ceres_cmd master| grep node_ip|awk '{print $3}'`
    if [ `tail -n 1 $cfgfile |grep server |wc -l` -ne 0 ]; then
        sed -i '$c server '$ip'' $cfgfile
    else
        echo "server $ip" >> $cfgfile
    fi
    systemctl restart ntpd
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
    systemctl stop ntpd
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



