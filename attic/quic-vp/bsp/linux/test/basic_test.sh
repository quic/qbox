#!/bin/bash

#
# Basic tests for the GKI Linux BSP on Virtual Platform
#
LOG="run.log"
DEBUG="/dev/null"


startvp() {
   sleep 1
   exec &> >(tee -a $LOG)
   vp --gs_luafile $VPTOP/conf.lua &
   VPID=$!
}

testnet() {
    sleep 10
    curl --retry 2 http://localhost:2280
    RC=$?
    if [ $RC -eq 0 ]
    then
        echo "PASS virtionet"
    else
        echo "FAIL virtionet"
	FAIL=1;
    fi

}
testprompt() {
    grep "Welcome to Buildroot" $LOG 2>&1 >>$DEBUG
    RC=$?
    if [ $RC -eq 0 ]
    then
        echo "PASS prompt"
    else
        echo "FAIL prompt"
	FAIL=1;
    fi
}
testblock() {
    grep "/dev/vda on /vda" $LOG 2>&1 >>$DEBUG
    RC=$?
    if [ $RC -eq 0 ]
    then
        echo "PASS virtioblock"
    else
        echo "FAIL virtioblock"
	FAIL=1;
    fi
}

#
# Options and HELP
#
help() {
    printf "Usage: %s \n" $0
    printf "\t-v Virtual platform install directory, conf.lua should be in this directory\n"
}

while getopts ":hv:" option
do
	case $option in
		h) # Help
			help
			exit 0;;
		v) # VP install
			VPTOP="$OPTARG";;
	esac
done

if [ -a $VPTOP ]
then
    echo Using Virtual Plathform directory: $VPTOP
else
    echo Virtual Plathform directory: $VPTOP not found
    exit 1
fi


#
# Cleanup prior to starting
# Kill any existing vp sessions prior to starting.
#
pkill vp
rm -f $LOG
if [ $DEBUG != "/dev/null" ]
then
    rm -f $DEBUG
fi



#
# Run
#
startvp
FAIL=0

testnet
testprompt
testblock
if [ $FAIL -eq 0 ]
then
    echo "All tests PASSED"
    EXIT_CODE=0
else
    echo "Some tests FAILED"
    EXIT_CODE=1
fi


kill $VPID
stty sane
exit $EXIT_CODE
