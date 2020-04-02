#!/bin/bash

CM_DIR="/usr/cm/"

if [ ! -d /var/cm/script ]; then
    mkdir -p /var/cm/script
fi

cp "$CM_DIR"script/*.sh /var/cm/script
cp "$CM_DIR"script/*.py /var/cm/script

chmod 755 /var/cm/script/*

if [ ! -d /var/cm/static ]; then
    mkdir -p /var/cm/static
    cp -rf "$CM_DIR"static/* /var/cm/static
fi

if [ ! -d /var/cm/log ]; then
    mkdir -p /var/cm/log
fi

if [ ! -d /var/cm/data ]; then
    mkdir -p /var/cm/data
fi


if [ ! -f /usr/sbin/cm_multi_exec ]; then
    ln -s /var/cm/script/cm_multi_exec.py /usr/sbin/cm_multi_exec
fi

if [ ! -f /usr/sbin/cm_multi_send ]; then
    ln -s /var/cm/script/cm_multi_send.py /usr/sbin/cm_multi_send
fi

if [ ! -f /usr/sbin/cm_disk ]; then
    ln -s /var/cm/script/cm_disk.sh /usr/sbin/cm_disk
fi

if [ ! -f /usr/sbin/cm_topo ]; then
    ln -s /var/cm/script/cm_topo.sh /usr/sbin/cm_topo
fi

if [ ! -f /usr/sbin/clumgt ]; then
    ln -s /var/cm/script/cm_cnm_clumgt.sh /usr/sbin/clumgt
fi

if [ ! -f /usr/lib/systemd/system/ceres_cm.service ]; then
    cp -rf "$CM_DIR"build/ceres_cm.service /lib/systemd/system/
    cp -rf "$CM_DIR"build/cm_multi_server.service /lib/systemd/system/
    systemctl daemon-reload
    systemctl enable ceres_cm
    systemctl enable cm_multi_server
fi

/var/cm/script/alarm_tool.sh
cp /usr/bin/ceres_* /usr/local/bin

