#!/bin/bash

echo "stop GUI"
guictl disable
sleep 1
echo "stop cm"
svcadm disable ceres_cm
sleep 1

echo "stop cluster nas"
zfs multiclus -vd
echo "stop cluster san"
zfs clustersan disable -c -f
    
echo "rm -f /var/cm/data/cm_*.db"
rm -f /var/cm/data/cm_*.db
sleep 1
echo "rm -f /var/cm/data/pmm_backup/*"
rm -f /var/cm/data/pmm_backup/*
sleep 1
echo "rm cm_cluster.ini"
rm -f /var/cm/data/*.ini
sleep 1
echo "start cm"
svcadm enable ceres_cm
sleep 1
echo "start GUI"
guictl enable

echo "disable clusterd"
svcadm disable clusterd

echo "Clean Complete. Now please reboot your machine."
