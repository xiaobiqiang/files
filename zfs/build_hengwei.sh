#!/bin/bash
cd cmd/cm/build/
./makeam.sh
cd -
cp module/Makefile_hengwei.in module/Makefile.in
cp etc/systemd/system/fc-boot.service.in_hengwei etc/systemd/system/fc-boot.service.in
./autogen.sh
./configure --enable-hengwei=yes --with-spl=$1
make -j 16 && make install
