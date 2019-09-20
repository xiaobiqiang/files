#!/bin/bash

source '/var/fm/fmd/script/config'
main()
{
        local paranum=$#
        if [ $paranum -eq 1 ];then
                ipmitool -H $HOST -U $USER -P $PASSWD sdr|grep $1|awk -F'|' '{print substr($3,2)}'
                return;
        fi
        if [ $paranum -eq 0 ];then
                ipmitool -H $HOST -U $USER -P $PASSWD sdr|awk -F'|' '{print substr($1,1)"|"substr($3,2)}'
                return;
        fi
}

main $1
exit $?
