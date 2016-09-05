# DGraph
  DGraph is a large-scale graph processing system.
  
  It gives you simple APIs and very fast computing speed.
  
  Its core algorithm is SCC-DAG execution model.

## Start Up
### Install
``` shell
mkdir DGraph
cd DGraph
mkdir bin
mkdir source
cd source
git clone <DGraph git URL>
make
make install
```
After install, the execute files are in bin dir

### Run
First, we need to generate DGraph files(preprocessing) from a certain original graph dataset. After that, we can run many algortihm on the DGraph files.
#### Preprocessing graph dataset
1. Prepare a dataset
2. 
  You can get dataset from [SNAP](http://snap.stanford.edu/data/)
    
  Dataset format that DGraph support: 
    
    (1) Adj-format:
    
      eg:
      
        
        0 232 3423 324
        1 100 234 232 3324
        2 324 45 65 2324
        ...
        
    (2) Edge-list:
    
      eg:
      
        
        0 232
        0 3423
        0 324
        1 100
        1 234
        1 232
        ...
        

2. Generate DGraph files
  ``` shell
  cd DGraph
  mkdir test
  ./bin/build.sh <dataset.txt> test
  ```
  And this dataset's DGraph files are:

  ```
  bor_pg_vtxs_in.bin
  bor_pg_vtxs_out.bin
  calc_vid_order.bin
  degree_pg_vtxs_in.bin
  degree_pg_vtxs_out.bin
  index_pg_vtxs_in.bin
  index_pg_vtxs_out.bin
  pg_id_to_vid.bin
  pg_vid_to_id.bin
  scc_group.bin
  scc_topo_level.bin
  ```
  
#### Run algorithm
Eg: PageRank
``` shell
#See Usage:
./bin/pagerank
```
You get
``` shell
Usage ./pagerank <data dir> <generate value to dir> <thread num(default processor num)><print top(default 10)>
```
Run PageRank
``` shell
./bin/pagerank test test
```
And you get result like
``` shell
Loading SCC DAG data...
initialize vtxs data (vtxs num: 735322)
Initialize value container
Initialize scc data
Load success.
Thread num: 16
asyn para run

(Time: 1473046251624 delta: 363)
Top value:
0:	0.0418388
73532:	0.0838599
147064:	0.118305
220596:	0.273202
294128:	0.037107
367660:	0.0410002
441192:	0.210406
514724:	0.0498573
588256:	0.120761
661788:	0.0442025
```

### Implement a self-designed graph algorithm

## DGraph Implementation
