# /bin/bash

if [ -z $1 ]
then
	echo "Usage: build.sh <dataset_file>  <generate_dir>"
	echo "Example: ./bin/build.sh database/amazon.txt test/amazon/"
	exit
fi

mkdir $2 -p

DIR=$(cd "$(dirname "$0")"; pwd)

$DIR/build_normal $1 $2
$DIR/build_scc_dag $2
