#! /bin/bash

# $1: algorithm name
# $2: test/dataset
# $3: test/result
# $4: argument

#sync; echo 3 > /proc/sys/vm/drop_caches

DIR=$(cd "$(dirname "$0")"; pwd)


function run()
{
	RD1=$($DIR/get_io_read.sh)
	WT1=$($DIR/get_io_write.sh)
	$1 $2 $3 $4 $5 $6 $7
	RD2=$($DIR/get_io_read.sh)
	WT2=$($DIR/get_io_write.sh)
	echo "Disk Read: "$(expr $RD2 - $RD1)"(KB)" 
	echo "Disk Write: "$(expr $WT2 - $WT1)"(KB)" 
	echo "--------------------------------------"
	echo
	
}


#echo "------------Load Data----------------- " $1_sd_async $2 $3
#$DIR/$1_sd_async $2 $3 $4

echo "------------Normal Sync--------------- " $1_normal_sync $2 $3
run $DIR/$1_normal_sync $2 $3 $4

echo "------------SD Sync------------------- " $1_sd_sync $2 $3
run $DIR/$1_sd_sync $2 $3 $4
echo "------------Normal Async-------------- " $1_normal_async $2 $3
run $DIR/$1_normal_async $2 $3 $4
echo "------------SD Async------------------ " $1_sd_async $2 $3
run $DIR/$1_sd_async $2 $3 $4
echo "------------End-----------------------"


