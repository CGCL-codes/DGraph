#ifndef SCC_BUILD_H
#define SCC_BUILD_H

#include "type.h"
#include "debug.h"
#include "gfile.h"
#include "sorter.h"

#include <iostream>
#include <cstdlib>
#include <set>
#include <vector>
#include <stack>
#include <algorithm>
#include <cmath>

#include <limits.h>

using namespace std;

const int MAX_INT = INT_MAX;

class Vtx
{
public:
	ID id;
	bor_cnt_t outi;
	cnt_t out_cnt;
	ID* bor;
};

//Vtx initralize(Use construct function will occupy much menory)
void vtx_init(Vtx* v, VtxUnit<ID>* vu)
{
	v->id = *(vu->id_p);
	v->outi = (bor_cnt_t)*(vu->cnt_p) - 1;
	v->out_cnt = *(vu->cnt_p);
	v->bor = vu->bor;
	delete vu;
}

void vtx_init(Vtx* v, ID &u)
{
	v->id = v->outi = v->out_cnt = 0;
	v->bor = new ID(u);
}

class SCC_Vtx
{
public:
	ID id, from_id, to_id;
	vector<Vtx*> scc_group;
	vector<ID> scc_group_id;
	cnt_t outcnt;
	ID* outbr;
	SCC_Vtx(ID id): id(id), from_id(ID_NULL), to_id(ID_NULL), const_incnt(0), incnt(0){};

	//topo sort
	cnt_t const_incnt, incnt;
};

template<class T>
T &  min(T &a, T &b){return a < b ? a : b;};


//Read file
void read_dataset_adj_no_size(char *fname, RawFile<ID> &rfile);
void read_dataset_hlist(char *fname, RawFile<ID> &rfile);

//SCC
void build_scc(VtxsFile<ID> &fvtxs);
void build_scc_graph(VtxsFile<ID> &fscc_vtxs);
void tarjan(ID vi);

//Topo sort
const cnt_t BIG_SCC_SIZE = 16384; // 10240 is quicker than 512; test on webbase-2001.txt 10240:81673 512:101154
void build_toposort();
void init_topo_incnt();
void init_topo_sort();

#endif
