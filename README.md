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

    You can get dataset from [SNAP](http://snap.stanford.edu/data/)
    
    Dataset format that DGraph support: 
    
    (1) Adj-format:
    
      eg:
      
        ``` 
        0 232 3423 324
        1 100 234 232 3324
        2 324 45 65 2324
        ...
        ```
    (2) Edge-list:
    
      eg:
      
        ```
        0 232
        0 3423
        0 324
        1 100
        1 234
        1 232
        ...
        ```

2. Generate DGraph files
``` shell
cd DGraph
mkdir test
./bin/build.sh <dataset.txt> test
```

And this dataset's DGraph files are:

```
ls
```

#### Run algorithm


### Implement a self-designed graph algorithm

## DGraph Implementation
