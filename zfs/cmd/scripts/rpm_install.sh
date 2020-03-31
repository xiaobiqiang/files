#!/bin/bash
function install_disk()
{
    mkdir -p /var/fm
    cp /usr/local/sbin/sg_led_disk.sh /var/fm/
    chmod 755 /var/fm/sg_led_disk.sh
}

install_disk
