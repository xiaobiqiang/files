#!/bin/bash

./autogen.sh
./configure
make -j32 && make install
