#!/bin/bash
function cm_cnm_ntp_server()
{
    #cp /etc/inet/ntp.server /etc/inet/ntp.conf
    #echo "server 127.127.1.0" >>  /etc/inet/ntp.conf
    #svcadm disable ntp
    #svcadm enable ntp
    return 0
}

function cm_cnm_ntp_client()
{
	ip=`ceres_cmd master| grep node_ip|awk '{print $3}'`
	cp /etc/inet/ntp.client /etc/inet/ntp.conf
	echo "server $ip" >>  /etc/inet/ntp.conf
	svcadm disable ntp
	svcadm enable ntp
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
	svcadm disable ntp
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



