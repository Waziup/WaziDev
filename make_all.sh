#Compile all examples and generate a result file
INOS=`find ./examples -name "*.ino"`
BASE=$PWD
ARDUINO_DIRECTORIES_USER=$PWD

set +e #avoid that a single failure stops the script

for f in $INOS
do
        echo Compiling $f ...
        juLog arduino-cli compile -p /dev/ttyUSB0 --fqbn arduino:avr:pro $f 2>&1 > /dev/null
        #RES=$?
        #echo "$f, $RES, $ERR" >> test_results.csv
done
echo
echo "##### FInished compiling #####"
echo
