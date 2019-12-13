
INOS=`find ./examples -name "*.ino"`
BASE=$PWD
set -e

for f in $INOS
do
        echo Compiling $f ...
        DIR=`dirname $f`
	cd $DIR
        make
        cd $BASE
done
echo
echo "##### All sketches compiled! #####"
echo
