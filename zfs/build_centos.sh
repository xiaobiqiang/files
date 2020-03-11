#!/bin/bash
cd cmd/cm/build/
chmod 755 ./makeam.sh
./makeam.sh
cd -
cp module/Makefile_centos.in module/Makefile.in
./autogen.sh
./configure --with-spl=$1
make && make install
