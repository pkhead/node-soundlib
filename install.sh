#!/usr/bin/bash
if [ ! -f "libsoundio/build" ]
then
    mkdir libsoundio/build
fi
cd libsoundio/build
cmake ..
make