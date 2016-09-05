#/bin/sh
NAME=$(date "+%Y-%m-%d-%H-%M-%S")-$1.tar 
tar -cvf $NAME $1
cp $NAME backup/
#scp $NAME /home/liao/shixiang/SCC_DAG/backup/
scp $NAME /media/home/liao/shixiang/SCC_DAG/backup/
rm $NAME
echo backup success! $NAME
