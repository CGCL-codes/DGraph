#! /bin/bash

# $1: datasets_dir
# $2: result_dir

#sync; echo 3 > /proc/sys/vm/drop_caches

DIR=$(cd "$(dirname "$0")"; pwd)

ALG="pagerank bfs sssp k-core"
DATASET=(enwiki soc-livejournal soc-pokec webbase twitter uk2007 uk-union)
AGM1=(2692466 3877322 1632795 34441 1526 13967040 41199424)

DATA_NUM=${#DATASET[@]}
echo $DATA_NUM



for((i=0; i < $DATA_NUM; i++))
{
	DATAi=${DATASET[$i]};

	AGMi=${AGM1[$i]};

	echo "########### " $DATAi
	sync; echo 3 > /proc/sys/vm/drop_caches
	echo "## Load data"
	echo "run---> " $DIR/bfs_sd_sync $1/$DATAi $2 $AGMi
	$DIR/bfs_sd_sync $1/$DATAi $2 $ARMi
	echo "## Load data end"

	for ALGi in $ALG
	do
		#Only bfs and sssp need to specify start vtx_id(AGMi)
		if [ "$ALGi" != "bfs" ] && [ "$ALGi" != "sssp" ]
		then
			AGMi=" "
		else
			AGMi=${AGM1[$i]}
		fi
		echo "run---> " $DIR/run_compare_algorithm.sh $ALGi $1/$DATAi $2 $AGMi
		$DIR/run_compare_algorithm.sh $ALGi $1/$DATAi $2 $AGMi
	done

}


