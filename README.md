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
After install, the execute files are in bin dir like
```
bfs
build_normal
build_scc_dag
build.sh
k-core
pagerank
sssp
```

### Run
First, we need to generate DGraph files(preprocessing) from a certain original graph dataset. After that, we can run many algortihm on the DGraph files.
#### Preprocessing graph dataset
1. Prepare a dataset
 
  You can get dataset from [SNAP](http://snap.stanford.edu/data/)
    
  Dataset format that DGraph support: 
    
    (1) Adj-format: 
    
        0 232 3423 324
        1 100 234 232 3324
        2 324 45 65 2324
        ...
        
    (2) Edge-list:
    
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
And you get results like
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

### Write a self-designed graph algorithm
1. Write a derived class of base class ""
2. Write Constructor(), init() and update() in derived template

Eg BFS:
``` cpp
template <class App>
class BFSApp : public App
{
private:
	ID root;
public:
	BFSApp(ID root, string dir, string value_dir): App(dir, string("bfs_value.bin"), value_dir), root(root){};

	int min(int a, int b){return a < b ? a : b;}	

	void init(ID id, typename App::value_t &value)
	{
		if(id == root)	value = 0;
		else value = INT_MAX - 1;
	}

	bool update(ID virtual_id, typename App::bor_t inbor, typename App::bor_t outbor, bor_cnt_t in_cnt, bor_cnt_t out_cnt,
		const typename App::values_t rvalue, typename App::values_t wvalue)
	{
		int d = rvalue[virtual_id];

		ID idval = 0;
		PG_Foreach(inbor, idval)
		{
			d = min(d, rvalue[idval] + 1);
		}

		bool is_cvg = (d == wvalue[virtual_id]);
		wvalue[virtual_id] = d;
        return is_cvg;
	}
};

int main(int argc, char *argv[])
{
    if (argc != 3 && argc != 4 && argc != 5)
    {
            cout << "Usage ./bfs <data dir> <generate value to dir> <root(default 0)> <bfs.nt top(default 10)>" << endl;
            return 0;
    }

	ID root = ((argc == 4 || argc == 5) ? atoll(argv[3]) : 0);
	int top_num = (argc == 5 ? top_num = atoi(argv[4]) : 10) ;

	string dir(argv[1]), to_dir(argv[2]);
	BFSApp<SccDagApp<int>> bfs(root, dir, to_dir);

	cout << "sync para run" << endl; get_time(false);
	bfs.reset();
	bfs.para_run();

	cout << "asyn para run" << endl; get_time(false);
	bfs.reset(false);
	bfs.para_run();

	get_time(); cout << endl;
	bfs.print_top(top_num);

	return 0;
}

```



## DGraph Implementation
