#!/bin/bash

#only root
MYID=`id $1 | sed -e 's/(.*$//' -e 's/^uid=//'`
if [ $MYID -ne 0 ] ; then
        echo "only root can execute this script."
        echo "exited without initializing"
        exit
fi

#default values of variable
PRODIGYCONF=/etc/prodigy.conf
ROLE="master"
HOSTID=
HOSTTYPE=DF5000C
MASTERIPADDR=192.168.0.50
 SLAVEIPADDR=192.168.0.51
DEFAULTMASK=255.255.255.0

clusterflag=0
sanflag=0
CONTROLLER_AMOUNT=2
nicpath=
osversion=


function dumpconfiglist
{
	echo "default configure list"
	echo ""
	cat $PRODIGYCONF
	echo ""
	echo ""
	echo "     role:$ROLE"
	echo "   hostid:$HOSTID"
	echo " hostname:$HOSTNAME"
	echo "ipaddress:$IPADDR"
}

# create serialno file
function create_serialno
{
CONOPT=
gettext "Enter host controller serial number: "
read CONOPT

if [ ! -z "${CONOPT}" ];then
        echo "SN: ${CONOPT}"  > /etc/serialno
fi
}

function setcontrollerdefault
{
CONOPT=
gettext "Enter host controller role:(1:master; 2:slave; default:master):"
read CONOPT
rm -f /etc/hostname.*
if  [ "${CONOPT}" != "2" ];then
	ROLE="master"
	HOSTID=1
	HOSTNAME="dfa"
	IPADDR=$MASTERIPADDR
	IPADDRixgbe1=10.10.2.1
else
	ROLE="slave"
	HOSTID=2
	HOSTNAME="dfb"
 	IPADDR=$SLAVEIPADDR
	IPADDRixgbe1=10.10.2.2
fi
	HOSTTYPE=DF5000C

	[ -f $PRODIGYCONF ]&&rm -f $PRODIGYCONF
	touch $PRODIGYCONF

echo ""
echo ""
cat > $PRODIGYCONF<<_PRODIGY_
CONTROLLER_AMOUNT=2
CONTROLLER_VENDOR=SBB
CONTROLLER_TYPE=B10
SYSTEM_STMF_KS=ENABLE
ENCLOSURE_VENDOR=LS
ENCLOSURE_SLOT=24
ENCLOSURE_DISK_TYPE=SAS
ENCLOSURE_INTERFACE_TYPE=SAS
ENCLOSURE_ENCRYPT_TYPE=OFF
ENCLOSURE_AMOUNT=0
_PRODIGY_

	dumpconfiglist

}


function create_prodigy_mannel_conf
{
echo "Create prodigy.conf!"
[ -f $PRODIGYCONF ]&&rm -f $PRODIGYCONF
touch $PRODIGYCONF

response=
gettext "Do you wish to use DEFAULT configure for prodigy.conf? [y/n] (default: y) "
read response

if [ -z $response ]||[ $response != "n" -a $response != "N" ];then
cat > $PRODIGYCONF<<_PRODIGY_
CONTROLLER_AMOUNT=2
CONTROLLER_VENDOR=SBB
CONTROLLER_TYPE=B10
SYSTEM_STMF_KS=ENABLE
ENCLOSURE_VENDOR=LS
ENCLOSURE_SLOT=24
ENCLOSURE_DISK_TYPE=SAS
ENCLOSURE_INTERFACE_TYPE=SAS
ENCLOSURE_ENCRYPT_TYPE=OFF
ENCLOSURE_AMOUNT=0
_PRODIGY_
else
#input number of controler
CONOPT=
if [ $clusterflag -ne 3 ];then
	gettext "Enter host controller number: (Type a number during 1-16; default:2):"
	read CONTROLLER_AMOUNT
	if [ -z $CONTROLLER_AMOUNT ]||[ $CONTROLLER_AMOUNT -gt 16 ]||[ $CONTROLLER_AMOUNT -lt 0 ];then
		printf "Use default 2\n"
		CONTROLLER_AMOUNT=2
	fi
	if  [ -n "${CONTROLLER_AMOUNT}" ];then
		cat $PRODIGYCONF |echo CONTROLLER_AMOUNT=${CONTROLLER_AMOUNT} >> $PRODIGYCONF
	fi
else
	gettext "Enter host controller number: (1 or 2; default:2):"
	read CONOPT

	if  [ "${CONOPT}" == "1" ];then
		cat $PRODIGYCONF |echo 'CONTROLLER_AMOUNT=1' >> $PRODIGYCONF
	else
		printf "Use default 2\n"
		cat $PRODIGYCONF |echo 'CONTROLLER_AMOUNT=2' >> $PRODIGYCONF
	fi
fi

CONOPT=
gettext "Enter host controller vendor: (1:Sun Fire Server; 2:SBB, 3:other; default:SBB):"
read CONOPT

if [ -z "${CONOPT}"  ] ||[ "${CONOPT}" != "1" ] && [ "${CONOPT}" != "2" ] && [ "${CONOPT}" != "3" ];then
        printf "Use default SBB\n"
fi

if [ "${CONOPT}" == "3" ];then
        cat $$PRODIGYCONF |echo 'CONTROLLER_VENDOR=OTHER' >> $PRODIGYCONF
elif [ "${CONOPT}" == "1" ];then
        cat $PRODIGYCONF |echo 'CONTROLLER_VENDOR=FIRE' >> $PRODIGYCONF
	CONOPT=
	gettext "Enter host controller type: (1:X4400; 2:X4200; default:x4200):"
	read CONOPT

	if [ -z "${CONOPT}"  ] ||[ "${CONOPT}" != "1" ] && [ "${CONOPT}" != "2" ];then
		printf "Use default x4200\n"
	fi

	if [ "${CONOPT}" == "1" ];then
		cat $PRODIGYCONF |echo 'CONTROLLER_TYPE=S40' >> $PRODIGYCONF
	else
		cat $PRODIGYCONF |echo 'CONTROLLER_TYPE=S20' >> $PRODIGYCONF
	fi
else
        cat $PRODIGYCONF |echo 'CONTROLLER_VENDOR=SBB' >> $PRODIGYCONF
        cat $PRODIGYCONF |echo 'CONTROLLER_TYPE=B10' >> $PRODIGYCONF
fi

#

CONOPT=

gettext "Enter system stmf ksocket: (1:enable; 2:disable; default:enable):"
read CONOPT

if [ "${CONOPT}" != "1" ] && [ "${CONOPT}" != "2" ] ;then
        printf "Use default enable\n"
fi

if [[ "${CONOPT}" == "2" ]];then
        cat $PRODIGYCONF |echo 'SYSTEM_STMF_KS=DISABLE' >> $PRODIGYCONF
else
        cat $PRODIGYCONF |echo 'SYSTEM_STMF_KS=ENABLE' >> $PRODIGYCONF
fi

#

CONOPT=
gettext "Enter Enclosure vendor: (1:xyrate; 2:lsi; 3:dothill; 4: other; default:lsi):"
read CONOPT

if [ "${CONOPT}" != "1" ] && [ "${CONOPT}" != "2" ]  && [ "${CONOPT}" != "3" ]  && [ "${CONOPT}" != "4" ];then
        printf "Use default lsi\n"
fi

if [[ "${CONOPT}" == "4" ]];then
        cat $PRODIGYCONF |echo 'ENCLOSURE_VENDOR=OTHER' >> $PRODIGYCONF
elif  [[ "${CONOPT}" == "3" ]];then
        cat $PRODIGYCONF |echo 'ENCLOSURE_VENDOR=DH' >> $PRODIGYCONF
elif  [[ "${CONOPT}" == "1" ]];then
        cat $PRODIGYCONF |echo 'ENCLOSURE_VENDOR=XY' >> $PRODIGYCONF
else
        cat $PRODIGYCONF |echo 'ENCLOSURE_VENDOR=LS' >> $PRODIGYCONF
fi
#

CONOPT=
gettext  "Enter enclose slot number: (1:12; 2:16; 3:24; 4:48; default:24) :"
read CONOPT

if [ "${CONOPT}" != "1" ] && [ "${CONOPT}" != "2" ]  && [ "${CONOPT}" != "3" ] && [ "${CONOPT}" != "4" ];then
        printf "Use default 24\n"
fi

if [[ "${CONOPT}" == "4" ]];then
        cat $PRODIGYCONF |echo 'ENCLOSURE_SLOT=48' >> $PRODIGYCONF
elif [[ "${CONOPT}" == "2" ]];then
        cat $PRODIGYCONF |echo 'ENCLOSURE_SLOT=16' >> $PRODIGYCONF
elif [[ "${CONOPT}" == "1" ]];then
        cat $PRODIGYCONF |echo 'ENCLOSURE_SLOT=12' >> $PRODIGYCONF
else
        cat $PRODIGYCONF |echo 'ENCLOSURE_SLOT=24' >> $PRODIGYCONF
fi

#

CONOPT=
gettext "Enter enclose disk type: (1:FC; 2:SAS; 3:SATA; 4:SSD; default:SAS):"
read CONOPT

if [ "${CONOPT}" != "1" ] && [ "${CONOPT}" != "2" ]  && [ "${CONOPT}" != "3" ] && [ "${CONOPT}" != "4" ];then
        printf "Use default SAS\n"
fi

if [[ "${CONOPT}" == "4" ]];then
        cat $PRODIGYCONF |echo 'ENCLOSURE_DISK_TYPE=SSD' >> $PRODIGYCONF
elif  [[ "${CONOPT}" == "3" ]];then
        cat $PRODIGYCONF |echo 'ENCLOSURE_DISK_TYPE=SATA' >> $PRODIGYCONF
elif  [[ "${CONOPT}" == "1" ]];then
        cat $PRODIGYCONF |echo 'ENCLOSURE_DISK_TYPE=FC' >> $PRODIGYCONF
else
        cat $PRODIGYCONF |echo 'ENCLOSURE_DISK_TYPE=SAS' >> $PRODIGYCONF
fi
#
CONOPT=
gettext  "Enter enclose disk interface type: (1:FC; 2:MINISAS;  default:MINISAS):"
read CONOPT

if [ "${CONOPT}" != "1" ] && [ "${CONOPT}" != "2" ]   ;then
        printf "Use default SAS\n"
fi

if [[ "${CONOPT}" == "1" ]];then
        cat $PRODIGYCONF |echo 'ENCLOSURE_INTERFACE_TYPE=FC' >> $PRODIGYCONF
else
        cat $PRODIGYCONF |echo 'ENCLOSURE_INTERFACE_TYPE=SAS' >> $PRODIGYCONF
fi

#
CONOPT=
gettext "Enter system encryption type: (1:off; 2:sm1; 3: sm4; default:off):"
read CONOPT

if [ "${CONOPT}" != "1" ] && [ "${CONOPT}" != "2" ]  && [ "${CONOPT}" != "3" ];then
        printf "Use default off\n"
fi

if [ "${CONOPT}" == "3" ];then
        cat /etc/prodigy.conf |echo 'ENCLOSURE_ENCRYPT_TYPE=SM4' >> /etc/prodigy.conf
elif [ "${CONOPT}" == "2" ];then
        cat /etc/prodigy.conf |echo 'ENCLOSURE_ENCRYPT_TYPE=SM1' >> /etc/prodigy.conf
else
        cat /etc/prodigy.conf |echo 'ENCLOSURE_ENCRYPT_TYPE=OFF' >> /etc/prodigy.conf
fi

#
CONOPT=
gettext "Enter enclosure number(default:0): "
read CONOPT

if [ -z ${CONOPT} -o "${CONOPT}" -gt "40" ];then
        cat /etc/prodigy.conf |echo 'ENCLOSURE_AMOUNT=0' >> /etc/prodigy.conf
        printf "Use default 0\n"
else
        cat /etc/prodigy.conf |echo 'ENCLOSURE_AMOUNT='${CONOPT} >> /etc/prodigy.conf
fi
fi
}

#hostname
function sethostname
{
	if [ $clusterflag -ne 3 ];then
        printf "Enter hostname: (default dfa)"
        read HOSTNAME
		if [ -z $HOSTNAME ];then
			HOSTNAME="dfa"
		fi

	else
		if [ $ROLE == "master" ];then
			printf "Enter hostname: (default dfa)"
			read HOSTNAME
			if [ -z $HOSTNAME ];then
				HOSTNAME="dfa"
			fi
			HOSTID=1
		else
			printf "Enter hostname: (default dfb)"
			read HOSTNAME
			if [ -z $HOSTNAME ];then
				HOSTNAME="dfb"
			fi
			HOSTID=2

		fi
	fi

# for deepin
	local os=`cat /etc/os-release | grep -w ID | cut -d '=' -f 2`
	if [ $os == deepin ]; then
		chmod -R 755 /usr/lib
		echo $HOSTNAME >/etc/hostname
		sed "s/sw6a/$HOSTNAME/g" -i /etc/hosts
	fi
	hostnamectl set-hostname --static $HOSTNAME
}

#hostid
function set_hostid
{
	gettext "Enter hostid: (Type a number during 1-${CONTROLLER_AMOUNT}, default: 1)"
	read HOSTID
	if [ -z $HOSTID ]||[ $HOSTID -gt ${CONTROLLER_AMOUNT} ]||[ $HOSTID -lt 0 ];then
		HOSTID=1
	fi
	sethostid $HOSTID >/dev/null 2>&1
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

#integer i

declare -a octets
octets=$(IFS=. ; set -- ${value};  echo $*)
i=$(set -- ${octets[*]};  echo $#)

# There must be four octets
if [[ ${i} -ne 4 ]]; then
        return 1
fi

tmp='x'
# Each must be numeric and less than 256
for octet in ${octets[*]}
do
	if [ $tmp == 'x' ];then
		tmp=$octet
	fi
        is_numeric ${octet} || return 1
        if [[ ${octet} -gt 255 ]]; then
                return 1
        fi
done

# The first must not be zero
if [[ $tmp == "0" ]];  then
        return 1
fi

# Okay
return 0
}

function is_ipv4_dot_notation_for_mask
{
typeset value=${1}

typeset octets
typeset octet

#integer i

declare -a octets
octets=$(IFS=. ; set -- ${value};  echo $*)
i=$(set -- ${octets[*]};  echo $#)

# There must be four octets
if [[ ${i} -ne 4 ]]; then
        return 1
fi

tmp='x'
# Each must be numeric and less than 256
for octet in ${octets[*]}
do
	if [ $tmp == 'x' ];then
		tmp=$octet
	fi
        is_numeric ${octet} || return 1
        if [[ ${octet} -gt 255 ]]; then
                return 1
        fi
done

# Okay
return 0
}

#OTHERS OF OLD SYSTEMINIT.SH
function init_cluster_conf
{
	if [ ! -f /usr/sbin/cluster_init.sh ];then
		touch /usr/sbin/cluster_init.sh
	else
		rm /usr/sbin/cluster_init.sh
	fi

cat > /usr/sbin/cluster_init.sh <<_CLUSTERINIT_
#!/bin/bash

#
# In SAN system, to enable following commands if needed.
# When clusterd is started and SAN is enabled, we need 
# to make sure the standby paths of luns are are all ready,
# so we need to wait a moment for transmits of stmf ksocket. 
#
# default 5s, we can adjust it according to the total 
# number of luns' multipath. 
#
#sleep 5


#
# Cluster system, to initialize zfs mirror port
#
[ -f /var/cm/script/cm_cluster_init.sh ]&&/var/cm/script/cm_cluster_init.sh
_CLUSTERINIT_


	chmod 755 /usr/sbin/cluster_init.sh
}

#set ip address
function setipaddress
{
	cat /dev/null > /tmp/inittmp
	if [ "Ubuntu" == ${osversion:0:6} ] || [ "Debian" == ${osversion:0:6} ] || [ "Kylin" == ${osversion:0:5} ]; then
		echo "/etc/network/interfaces" > /tmp/nicpath
		ifconfig -a|grep Link|awk '{print $1}' >/tmp/inittmp
	elif [ "deepin" == ${osversion:0:6} ]; then
		echo "/etc/network/interfaces" > /tmp/nicpath
		ip a | grep mtu | grep -v lo | cut -d ' ' -f 2 | sed s/:// >/tmp/inittmp
	else
		echo "/etc/sysconfig/network-scripts/ifcfg-" > /tmp/nicpath	
		ifconfig -a|grep mtu|cut -d ':' -f 1 >/tmp/inittmp
	fi

	if [ -f /var/cm/static/nic.conf ];then
		rm /var/cm/static/nic.conf
	fi

	list=0
	while true
	do
		cat /tmp/inittmp|while read temp
		do
			if [ $temp == "lo" ];then
				continue
			fi
			list=`expr $list + 1`
			echo "[$list] $temp"
		done

		read -p "Config nic, Choice a nic: " choice
		printf "Enter ip address(Press Enter to use default ip):"
		read IPADDR
		if [ -z "$IPADDR" ] ; then
			if [ $ROLE == "slave" ];then
				IPADDR=$SLAVEIPADDR
				echo "Use default ip $IPADDR"
			else
				IPADDR=$MASTERIPADDR
				echo "Use default ip $IPADDR"
			fi
		fi

		is_ipv4_dot_notation "$IPADDR"
		if [ $? -ne 0 ]; then
			echo "Error in IP address, please retry"
			continue
		fi

		printf "Enter subnet mask(Press Enter to use default mask):"
		read MASK
		if [ -z "$MASK" ];then
			echo "$DEFAULTMASK" > /tmp/inittmp1
		else
			echo "$MASK" > /tmp/inittmp1
		fi
		is_ipv4_dot_notation_for_mask $MASK
		if [ $? -ne 0 ]; then
			echo "$DEFAULTMASK" > /tmp/inittmp1
			echo "Error in subnet mask, use default $DEFAULTMASK."
		fi

		dst=`cat /tmp/inittmp|head -$choice|tail -1`
		echo $dst >> /var/cm/static/nic.conf
		MASK=`cat /tmp/inittmp1`
		nicpath=`cat /tmp/nicpath`

		if [ "Ubuntu" == ${osversion:0:6} ] || [ "Debian" == ${osversion:0:6} ] || [ "Kylin" == ${osversion:0:5} ];then
			echo "CREATING $dst config file."
			cat >> $nicpath << _HSTNAME_
auto $dst
allow-hotplug $dst
iface $dst inet static
address $IPADDR
netmask $MASK
_HSTNAME_
		elif [ "deepin" == ${osversion:0:6} ]; then
			echo "CREATING $dst config file."
			local find=0
			# clear old inteface configuration
			while read line; do
				if [ X"auto $dst" == X"$line" ]; then
						find=1
				elif [[ X$line != X && $(echo $line | cut -d " " -f 1) == auto ]]; then
						find=0
				fi
				if [ $find -lt 1 ]; then
						echo $line >> ${nicpath}_tmp
				fi
			done < $nicpath
			mv ${nicpath}_tmp ${nicpath}
			
			cat >> $nicpath << _HSTNAME_
auto $dst
iface $dst inet static
address $IPADDR
netmask $MASK
_HSTNAME_
		else
			echo "creating $dst config file."
			cat > $nicpath$dst << _HSTNAME_
TYPE=Ethernet
BOOTPROTO=static
IPADDR=$IPADDR
NETMASK=$MASK
DEFROUTE=yes
IPV4_FAILURE_FATAL=no
NAME=$dst
DEVICE=$dst
ONBOOT=yes
_HSTNAME_
		fi

		printf "Do you want set another nic? [default : y]"
		read response

		if [ ! -z $response ]&&[ $response == "n" -o $response == "N" ];then
			break
		fi
	done

}

#Set persistent static route for partner ipmi
function set_partner_ipmi
{
	echo "zfs clustersan set ipmi=on" >> /tmp/cluster_init.sh
}

function init_ntb_eeprom
{
	gettext "Do you wish to initialize ntb eeprom? [y/n] (default: n) "
	read response
	if [ -z $response ]||[ $response != "y" -a $response != "Y" ];then
		echo "not initialize ntb eeprom !"
	else
		/usr/bin/ntb_eeprom -l /etc/enable_64M.bin
		return $?
	fi
}

function set_nas_rpc_nics
{
echo ""
echo ""
HOSTITEM=
IFNAME=
IPADDR=

while [ -z "$IFNAME" ]
do
	printf "Enter RPC port name (e.g. eth1, <CR> to exit): "
	read IFNAME
done

while true
do
	printf "Enter ip address(e.g. 192.168.1.50, <CR> to exit):"
	read IPADDR
	if [ -z "$IPADDR" ]; then
		continue
	fi

	is_ipv4_dot_notation "$IPADDR"
	if [ $? -ne 0 ]; then
		continue
	else
		break
	fi
done

echo creating /etc/hostname.$IFNAME...
cat > /etc/hostname.$IFNAME << HSTNAME
$IPADDR
netmask 255.255.255.0
up
HSTNAME

if [ ! -d /etc/fsgroup ];then
	mkdir -p /etc/fsgroup

fi
cat > /etc/fsgroup/rpc_port.conf << HSTNAME
rpc_port=$IFNAME;
rpc_addr=$IPADDR;
HSTNAME
}

function get_osversion
{
	if [ -f /etc/os-release ];then
		osversion=`cat /etc/os-release |grep PRETTY_NAME|cut -d '"' -f 2`
	fi

	if [ -z osversion ];then
		return -1
	else
		return 0
	fi
}

function add_prepare_gui
{
    local RCLOCALPATH=""
    local gui_script="/gui/prepare.sh"
    local check=0
    if [ ! -f  $gui_script ];then
        echo "$gui_script is not exist."
        return -1
    fi
    local os_type=`grep PRETTY_NAME /etc/os-release|cut -d '"' -f 2|grep deepin|wc -l`
    if [ $os_type -eq 0 ];then
        RCLOCALPATH="/etc/rc.d/rc.local"
        if [ -f  $RCLOCALPATH ];then
            check=`grep $gui_script $RCLOCALPATH|wc -l`
            if [ $check -eq 0 ];then
                sed "/exit/d" $RCLOCALPATH > /tmp/rc.tmp
                cat /tmp/rc.tmp > $RCLOCALPATH
                echo "/gui/prepare.sh &" >> $RCLOCALPATH
                chmod 755  $RCLOCALPATH
            fi
        else
            echo "add_prepare_gui, $RCLOCALPATH is not exist,now we create."
            echo "#!/bin/sh" >> $RCLOCALPATH
            echo "$gui_script" >> $RCLOCALPATH
            echo "exit 0" >> $RCLOCALPATH
            chmod 755  $RCLOCALPATH
            systemctl start rc-local
        fi
    else
        RCLOCALPATH="/etc/rc.local"
        if [ ! -f $RCLOCALPATH ];then
            echo "#!/bin/sh" >> $RCLOCALPATH
            echo "$gui_script" >> $RCLOCALPATH
            echo "exit 0" >> $RCLOCALPATH
            chmod 755  $RCLOCALPATH
            systemctl start rc-local
        else
            check=`grep $gui_script $RCLOCALPATH|wc -l`
            if [ $check -eq 0 ];then
                #sed "s:exit 0:$gui_script:g" $RCLOCALPATH > /tmp/rc.tmp
                sed "/exit/d" $RCLOCALPATH > /tmp/rc.tmp
                cat /tmp/rc.tmp > $RCLOCALPATH
                echo "$gui_script" >> $RCLOCALPATH
                echo "exit 0" >> $RCLOCALPATH
                systemctl start rc-local
            else
                echo "gui setting already exist"
            fi
        fi
    fi
    return $?
}

function deepin_unmanage_netdevice
{
	if [ "deepin" == ${osversion:0:6} ]; then
		for nd in `ip a | grep mtu | awk '{print $2}' | sed 's/://'`; do
			local mode=`ethtool $nd | grep 'Supported link modes:' | cut -d ' ' -f 6`
			if [ X10000baseT/Full == X$mode ] || [ X10baseT/Half == X$mode ]; then 
				local ether=`ifconfig $nd | grep ether | awk '{print $2}'`
				if [ `cat /etc/NetworkManager/NetworkManager.conf | grep "\[keyfile\]" | wc -l` -lt 1 ]; then
					echo "" >> /etc/NetworkManager/NetworkManager.conf
					echo '[keyfile]' >> /etc/NetworkManager/NetworkManager.conf
					echo "unmanaged-devices=mac:${ether};" >> /etc/NetworkManager/NetworkManager.conf
				else
					local origin=`cat /etc/NetworkManager/NetworkManager.conf | grep unmanaged-devices`
					if [ `echo "$origin" | sed "s/$ether//"` == "$origin" ]; then
						sed "s/${origin}/${origin}${ether};/" -i /etc/NetworkManager/NetworkManager.conf
					fi
				fi
			fi
		done
	fi
}

#main

get_osversion
if [ $? != 0 ];then
	osversion="CentOS Linux 7 (Core)"
	echo "get os version failed, default $osverison"
fi

echo
echo
#create /etc/serialno
response=
gettext "Do you wish to create /etc/serialno?[y/n] (default: n) "
read response

if [ -z $response ]||[ $response != "y" -a $response != "Y" ];then
	echo "not create /etc/serialno ! "
else
	create_serialno
fi

gettext "These settings are for (1.cluster san; 2.cluster nas; 3.not for cluster; default: 3): "
read response

if [ "$response" != "1" ] && [ "$response" != "2" ] && [ "$response" != "3" ] ;then
	clusterflag=3
	printf "Use default enable\n"
fi

if [ "$response" == "1" ];then
	clusterflag=1
elif [ "$response" == "2" ];then
	clusterflag=2
else
	clusterflag=3
fi

#	init_ntb_eeprom
#	if [ $? -ne 0 ];then
#		echo "initialize ntb eeprom failed and exit!"
#		exit 1
#	fi

echo""
echo""
create_prodigy_mannel_conf
if [ $? -eq 0 ];then
	echo "Create progidy.conf successfully!"
else
	echo "Create progidy.conf failed and exit!"
	exit 
fi

echo""
echo""
sethostname
if [ $? -eq 0 ];then
	echo "Set hostname successfully!"
else
	echo "Set hostname failed and exit!"
	exit 
fi

echo""
echo""
set_hostid
if [ $? -eq 0 ];then
	echo "Set hostid as $HOSTID successfully!"
else
	echo "Set hostid failed and exit!"
	exit 
fi

echo""
echo""
setipaddress
if [ $? -eq 0 ];then
	echo "Set ipaddress successfully!"
else
	echo "Set ipaddress failed and exit!"
	exit 
fi

echo""
echo""

#if [ $clusterflag -eq 2 ];then
#	echo""
#	echo""
#	response=
#	gettext "Do you wish to SET rpc nics for NAS? [y/n] (default: n) "
#	read response
#	if [ -z $response ]||[ $response != "y" -a $response != "Y" ];then
#		echo "not set rpc nics for NAS! "
#	else
#		set_nas_rpc_nics
#		if [ $? -eq 0 ];then
#			echo "Set nas rpc nics successfully!"
#		else
#			echo "Set nas rpc nics failed and exit!"
#			exit 
#		fi
#	fi
#fi

echo""
echo""
init_cluster_conf
if [ $? -eq 0 ];then
	echo "Init cluster successfully!"
else
	echo "Init cluster failed and exit!"
	exit 
fi

deepin_unmanage_netdevice

echo ""
echo ""
add_prepare_gui


echo Complete. Now please reboot your machine.

