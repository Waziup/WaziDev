#!/bin/bash

#Compile all examples and generate a result file
INOS=`find ./examples -name "*.ino"`
BASE=$PWD
ARDUINO_DIRECTORIES_USER=$PWD

set +e #avoid that a single failure stops the script

# Provides "juLog", used to create the JUnit XML output
source ./s2ju.sh

for f in $INOS
do
        echo Compiling $f ...
        juLog -name=$f arduino-cli compile -p /dev/ttyUSB0 --fqbn arduino:avr:pro:cpu=8MHzatmega328 $f 2>&1 > /dev/null
done
