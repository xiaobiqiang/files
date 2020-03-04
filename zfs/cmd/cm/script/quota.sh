#!/bin/bash

rm -rf /tmp/quota.txt
for k in `zfs list -H|awk '{print $1}'`
do 
                quota=`zfs get -H quota $k|awk '{printf $3}'`
                if [ "$quota" == "-" ];then
                        continue;
                fi
                used=`zfs get -H used $k|awk '{printf $3}'`
                echo $k"\t"$quota"\t"$used"##" >> /tmp/quota.txt
done
