#!/bin/bash

rm -rf /tmp/userquota.txt
fs_sum=`zfs list -H -t filesystem |awk '{print $1}'`
for k in `cat /etc/passwd|awk -F: '($3>99&&$3<50000){print $1}'`
do
        for i in $fs_sum
        do
                quota=`zfs get -H userquota@${k} $i |awk '{printf $3}'`
                if [ "$quota"X == "-"X ] || [ "$quota"X == ""X ];then
                        continue;
                fi
                #softquota=`zfs get -H softuserquota@${k} $i |awk '{printf $3}'`
                #if [ "$softquota"X == "-"X ] || [ "$softquota"X == ""X ];then
                #        continue;
                #fi
                softquota="0"
                userused=`zfs get -H userused@${k} $i |awk '{print $3}'`
                if [ "$userused"X == "-"X ] || [ "$userused"X == ""X ];then
                        continue;
                fi

                                if [ $1 ];then
                                        echo -e $k"\t"$i"\t"$quota"\t"$softquota"\t"$userused"##"
                                else
                                        echo -e $k"\t"$i"\t"$quota"\t"$softquota"\t"$userused"##" >> /tmp/userquota.txt
                                fi
        done
done
