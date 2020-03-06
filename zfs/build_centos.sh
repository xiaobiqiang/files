#!/bin/bash
cd cmd/cm/build/
./makeam.sh
cd -
cp module/Makefile_centos.in module/Makefile.in
cp etc/systemd/system/fc-boot.service.in_centos etc/systemd/system/fc-boot.service.in
./autogen.sh
./configure --with-spl=$1
make && make install
