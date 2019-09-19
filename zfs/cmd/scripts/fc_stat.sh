#!/bin/bash

DATA_DIR="/proc/spl/kstat/stmf"
TARGET_FILE="/tmp/target.tmp"
SYS_FILE="/tmp/sys.tmp"
PROC_NETDEV="/proc/net/dev"
PROC_NFSD="/proc/net/rpc/nfsd"
function stat_main()
{
#   echo "name      nread    nwritten reads    writes   wtime    wlentime wupdate  rtime    rlentime rupdate  wcnt     rcnt"
    rm -f ${TARGET_FILE}
    touch ${TARGET_FILE}
    local tgts=`ls $DATA_DIR |grep "stmf_tgt_f"`
    for tgt in $tgts
    do
        local tgt_id=`echo "$tgt" |awk -F'_' '{print $3}'`
        local tgt_name=`grep 'target-name' $DATA_DIR/$tgt |awk '{print $3}'`
        local tgt_data=`sed -n 3p $DATA_DIR/stmf_tgt_io_${tgt_id}`
        echo "${tgt_name} ${tgt_data}">>${TARGET_FILE}
    done
}
#==================================================================================
#			sum[0]:fc_r
#			sum[1]:fc_w
#			sum[2]:fc_nr
#			sum[3]:fc_nw
#			sum[4]:nic_nr
#			sum[5]:nic_nw
#			sum[6]:nic_r
#			sum[7]:nic_w
#==================================================================================
function stat_sys()
{
	rm -f ${SYS_FILE}
    	touch ${SYS_FILE}
	if [ ! -f "$TARGET_FILE" ]; then
		touch ${TARGET_FILE}
	fi
    if [ ! -f "$PROC_NFSD" ]; then
		local nfs_nr=0
        local nfs_nw=0
    else
        local nfs_nr=`cat $PROC_NFSD |grep "io"|awk -F' ' '{print $2}'`
        local nfs_nw=`cat $PROC_NFSD |grep "io"|awk -F' ' '{print $3}'`
	fi
	local sum=(0 0 0 0 0 0 0 0)
	local fc_name=`cat $TARGET_FILE |grep "wwn"|awk -F' ' '{print $1}'`
	local nic_name=`cat $PROC_NETDEV |grep ":"|awk -F':' '{print $1}'`
	
	for num in $fc_name
	do
		reads=`cat $TARGET_FILE|grep -w "$num"|awk -F' ' '{print $4}'`
		writes=`cat $TARGET_FILE|grep -w "$num" |awk -F' ' '{print $5}'`
		nread=`cat $TARGET_FILE|grep -w "$num" |awk -F' ' '{print $2}'`
		nwritten=`cat $TARGET_FILE|grep -w "$num" |awk -F' ' '{print $3}'`
		((sum[0]=${sum[0]}+$reads))
		((sum[1]=${sum[1]}+$writes))
		((sum[2]=${sum[2]}+$nread))
		((sum[3]=${sum[3]}+$nwritten))
	done
	for name in $nic_name
	do
		nic_nread=`cat $PROC_NETDEV|grep -w "$name" |awk -F' ' '{print $2}'`
		nic_nwritten=`cat $PROC_NETDEV|grep -w "$name" |awk -F' ' '{print $10}'`
		nic_reads=`cat $PROC_NETDEV|grep -w "$name" |awk -F' ' '{print $3}'`
		nic_writes=`cat $PROC_NETDEV|grep -w "$name" |awk -F' ' '{print $11}'`
		((sum[4]=$nic_nread+${sum[4]}))
		((sum[5]=$nic_nwritten+${sum[5]}))
		((sum[6]=$nic_reads+${sum[6]}))
		((sum[7]=$nic_writes+${sum[7]}))
	done
	echo "${sum[@]} $nfs_nr $nfs_nw">>${SYS_FILE}
}
function main()
{
	local paramnum=$#
	case $1 in 
		fc_stat)
		stat_main
		;;
		sys)
		stat_sys
		;;
		*)
        ;;
	esac
}
main $*
exit $?