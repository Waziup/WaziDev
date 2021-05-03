#Compile all examples and generate a result file
INOS=`find ./examples -name "Makefile"`
BASE=$PWD
set +e #avoid that a single failure stops the script

for f in $INOS
do
        echo Compiling $f ...
        DIR=`dirname $f`
	cd $DIR
        make
        RES=$?
        cd $BASE
        echo "$f, $RES" >> test_results.csv
done
echo
echo "##### FInished compiling #####"
echo
