#!/bin/bash

threshold_cfg_xml_file="/var/cm/static/comm/alarm_threshold_cfg.xml"
alarm_cfg_xml_file="/var/cm/static/comm/alarm_cfg.xml"
alarm_cfg_period_file="/var/cm/static/comm/alarm_period_cfg.xml"
alarm_cfg_db_file="/var/cm/data/cm_alarm.db"

SQLITE=sqlite3

CM_ALATM_TYPE_FAULT=0
CM_ALATM_TYPE_EVENT=1
CM_ALATM_TYPE_LOG=2

CM_ALATM_LVL_CRITICAL=0
CM_ALATM_LVL_MAJOR=1
CM_ALATM_LVL_MINOR=2
CM_ALATM_LVL_TRIVIAL=3

function alarm_init_db()
{
    local table_config="CREATE TABLE IF NOT EXISTS config_t (alarm_id INT,match_bits INT,param_num TINYINT,type TINYINT,lvl TINYINT,is_disable TINYINT)"
    local table_record="CREATE TABLE IF NOT EXISTS record_t (id BIGINT,alarm_id INT,report_time INT,recovery_time INT,param VARCHAR(255))"
    local table_cache="CREATE TABLE IF NOT EXISTS cache_t (id BIGINT,alarm_id INT,report_time INT,recovery_time INT,param VARCHAR(255))"
    local table_threshold="CREATE TABLE IF NOT EXISTS threshold_t (alarm_id INT, threshold INT, recoverval INT)"
    local period_cfg_t="CREATE TABLE IF NOT EXISTS period_cfg_t (alarm_id INT,cnt INT,tmout INT)"
    local period_t="CREATE TABLE IF NOT EXISTS period_t (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,alarm_id INT,cnt INT,tmout INT,param VARCHAR(255))"
    $SQLITE $alarm_cfg_db_file "$table_config"
    $SQLITE $alarm_cfg_db_file "$table_record"
    $SQLITE $alarm_cfg_db_file "$table_cache"
    $SQLITE $alarm_cfg_db_file "$table_threshold"
    $SQLITE $alarm_cfg_db_file "$period_cfg_t"
    $SQLITE $alarm_cfg_db_file "$period_t"
    if [ $? != 0 ]; then
        echo "create table fail"
        exit 1
    fi
}

function alarm_config_to_db()
{
    grep -w alarm $alarm_cfg_xml_file |while read line
    do
        val=$line
        if [ "X$val" = "X" ]; then
            echo "config null"
            exit 1
        fi
        alarm_id=`echo "$val" |sed 's/.*id="\(.*\)/\1/g' |awk -F'"' '{printf $1}'`
        alarm_type=`echo "$val" |sed 's/.*type="\(.*\)/\1/g' |awk -F'"' '{printf $1}'`
        alarm_level=`echo "$val" |sed 's/.*level="\(.*\)/\1/g' |awk -F'"' '{printf $1}'`
        alarm_pnum=`echo "$val" |sed 's/.*param_num="\(.*\)/\1/g' |awk -F'"' '{printf $1}'`
        alarm_match=`echo "$val" |sed 's/.*param_match="\(.*\)/\1/g' |awk -F'"' '{printf $1}'`
        alarm_dis=`echo "$val" |sed 's/.*is_disable="\(.*\)/\1/g' |awk -F'"' '{printf $1}'`
        if [ "$alarm_type" = "FAULT" ]; then
            alarm_type=$CM_ALATM_TYPE_FAULT
        elif [ "$alarm_type" = "EVENT" ]; then
            alarm_type=$CM_ALATM_TYPE_EVENT
        elif [ "$alarm_type" = "LOG" ]; then
            alarm_type=$CM_ALATM_TYPE_LOG
        else
            echo "type [$alarm_type] not support"
            exit 1
        fi
        
        if [ "$alarm_level" = "CRITICAL" ]; then
            alarm_level=$CM_ALATM_LVL_CRITICAL
        elif [ "$alarm_level" = "MAJOR" ]; then
            alarm_level=$CM_ALATM_LVL_MAJOR
        elif [ "$alarm_level" = "MINOR" ]; then
            alarm_level=$CM_ALATM_LVL_MINOR
        elif [ "$alarm_level" = "TRIVIAL" ]; then
            alarm_level=$CM_ALATM_LVL_TRIVIAL
        else
            echo "level [$alarm_level] not support"
            exit 1
        fi
        
        if [ "$alarm_dis" = "TRUE" ]; then
            alarm_dis=1
        elif [ "$alarm_dis" = "FALSE" ]; then
            alarm_dis=0
        else
            echo "is_disable [$alarm_dis] not support"
            exit 1
        fi

        if [ $alarm_type = $CM_ALATM_TYPE_FAULT ]; then
            cnt=`echo "$alarm_match" |awk -F',' '{print NF}'`
            sumval=0
            for((i=1;i<=$cnt;i++))
            do
                tmp=`echo "$alarm_match" |awk -F',' '{print $'$i'}'`
                sumval=$(($sumval|(1<<$tmp)))
            done
            alarm_match=$sumval
        else
            alarm_match=0
        fi
        local reccnt=`$SQLITE ${alarm_cfg_db_file} "SELECT COUNT(alarm_id) FROM config_t WHERE alarm_id=$alarm_id"`
        if [ "X$reccnt" == "X" ] || [ $reccnt -ne 0 ]; then
            continue
        fi
        sql="INSERT INTO config_t (alarm_id,match_bits,param_num,type,lvl,is_disable)"
        sql=$sql" VALUES ($alarm_id,$alarm_match,$alarm_pnum,$alarm_type,$alarm_level,$alarm_dis)"
        $SQLITE $alarm_cfg_db_file "$sql"
        if [ $? != 0 ]; then
            echo "insert [$sql] fail"
            exit 1
        fi
    done
}

function threshold_config_to_db()
{
    grep -w alarm_id $threshold_cfg_xml_file |while read line
    do
        val=$line
        if [ "X$val" = "X" ]; then
            echo "threshold config null"
            exit 1
        fi
        local alarm_id=`echo "$val" |sed 's/.*alarm_id="\(.*\)/\1/g' |awk -F'"' '{printf $1}'`
        local threshold=`echo "$val" |sed 's/.*threshold="\(.*\)/\1/g' |awk -F'"' '{printf $1}'`
        local recoverval=`echo "$val" |sed 's/.*recoverval="\(.*\)/\1/g' |awk -F'"' '{printf $1}'`
        
        local reccnt=`$SQLITE ${alarm_cfg_db_file} "SELECT COUNT(alarm_id) FROM threshold_t WHERE alarm_id=$alarm_id"`
        if [ "X$reccnt" == "X" ] || [ $reccnt -ne 0 ]; then
            continue
        fi
        sql="INSERT INTO threshold_t (alarm_id,threshold,recoverval)"
        sql=$sql" VALUES ($alarm_id,$threshold,$recoverval)"
        $SQLITE $alarm_cfg_db_file "$sql"
        if [ $? != 0 ]; then
            echo "insert [$sql] fail"
            continue
        fi
    done
}

function alarm_period_config_to_db()
{
    grep -w alarm_id ${alarm_cfg_period_file} |while read line
    do
        val=$line
        if [ "X$val" = "X" ]; then
            echo "period config null"
            continue
        fi
        #<alarm_id="10000014" report_count="1" recovery_time="180" />
        local alarm_id=`echo "$val" |sed 's/.*alarm_id="\(.*\)/\1/g' |awk -F'"' '{printf $1}'`
        local report_count=`echo "$val" |sed 's/.*report_count="\(.*\)/\1/g' |awk -F'"' '{printf $1}'`
        local recovery_time=`echo "$val" |sed 's/.*recovery_time="\(.*\)/\1/g' |awk -F'"' '{printf $1}'`
        
        local reccnt=`$SQLITE ${alarm_cfg_db_file} "SELECT COUNT(alarm_id) FROM period_cfg_t WHERE alarm_id=$alarm_id"`
        if [ "X$reccnt" == "X" ] || [ "X$reccnt" != "X0" ]; then
            $SQLITE $alarm_cfg_db_file "UPDATE period_cfg_t SET cnt=$report_count, tmout=$recovery_time WHERE alarm_id=$alarm_id"
            continue
        fi
        
        sql="INSERT INTO period_cfg_t (alarm_id,cnt,tmout)"
        sql=$sql" VALUES ($alarm_id,$report_count,$recovery_time)"
        $SQLITE $alarm_cfg_db_file "$sql"
        if [ $? != 0 ]; then
            echo "insert [$sql] fail"
            continue
        fi
    done
}

if [ ! -f ${alarm_cfg_db_file} ]; then
    exit 0
fi
alarm_init_db
alarm_config_to_db
threshold_config_to_db
alarm_period_config_to_db
exit 0

