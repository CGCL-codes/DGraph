#include "build.h"
#include "pathgraph.h"

using namespace std;


ID N = 0; // maxid

/*
 *	Build scc
 */

//build scc output
vector<Vtx*> vtxs;
vector<SCC_Vtx*> scc_vtxs;
vector<SCC_Vtx*> vtx_to_scc;

//tarjan global parameter
stack<Vtx*> scc_stk, scc_buff;
//QuickStack<Vtx*> scc_stk, scc_buff;
vector<ID> dfn, low;
vector<bool> in_stack;
ID scc_index = 0, scc_cnt = 0; //to generate dfn

void build_scc(VtxsFile<ID> &fvtxs)
{
	//Initralize
	vtxs.resize(N + 1);

	//#pragma omp parallel for schedule(static)
	for(ID i = 0; i <= N; i++)	
	{
		vtxs[i] = new Vtx();
		vtx_init(vtxs[i], fvtxs[i]);
	}
	dfn.resize(N + 1, 0);
	low.resize(N + 1, INT_MAX);
	vtx_to_scc.resize(N + 1);
	in_stack.resize(N + 1, false);

	//Tarjan algorithm
	cout << "Tarjan" << endl;
	for(ID i = 0; i <= N; i++)
	{
		if(dfn[vtxs[i]->id] == 0)
			tarjan(i);
	}

	//free space
	dfn.clear();
	low.clear();
	in_stack.clear();

/*	cnt_t scc_size = scc_vtxs.size();
	cout << "scc num: " << scc_size << endl;
	for(ID i = 0; i < scc_size; i++)
	{
		SCC_Vtx *scc = scc_vtxs[i];
		cout << "scc-" << scc->id << " : ";
		for(ID j = 0; j < scc->scc_group.size(); j++)
			cout << scc->scc_group[j]->id << " " ;
		cout << endl;
	}
*/
	//Make scc graph
	cout << "scc num: " << scc_vtxs.size() << endl;
}


void build_scc_graph(VtxsFile<ID> &fscc_vtxs)
{
	typedef set<SCC_Vtx*>::iterator sit;
	cnt_t scc_num = scc_vtxs.size();
	//combine scc vtx border
	//#pragma omp parallel for schedule(dynamic, 100)
	for(ID i = 0; i < scc_num; i++)
	{
		set<SCC_Vtx*> tmp_outbr;
		SCC_Vtx *scc_u = scc_vtxs[i];
		for(ID j = 0; j < scc_u->scc_group.size(); j++)
		{
			Vtx *u = scc_u->scc_group[j];
			for(ID k = 0; k < u->out_cnt; k ++)
			{
				SCC_Vtx *scc_v = vtx_to_scc[u->bor[k]];
				if(scc_u != scc_v)
					tmp_outbr.insert(scc_v);
			}
		}

		//Convert set to vector to reduce memory space.
		fscc_vtxs.add_vtx_start(scc_u->id);
		for(sit iter = tmp_outbr.begin(); iter != tmp_outbr.end(); iter++)
		{
			fscc_vtxs.add_bor((*iter)->id);
		}
		fscc_vtxs.add_vtx_end();

		scc_u->outcnt = tmp_outbr.size();
	}

	//Add file base addr to scc_u
	for(ID i = 0; i < scc_num; i++)
	{
		SCC_Vtx *scc_u = scc_vtxs[i];

		addr_t scc_base = fscc_vtxs.get_vtx_base_addr(scc_u->id);
		scc_u->outbr = VtxUnitData<ID>::bor(scc_base);
	}

}

void tarjan(ID vi)
{
	Vtx *u = new Vtx();
	vtx_init(u, vi);
	do
	{
		while(u->outi >= 0)
		{
			Vtx *v = vtxs[u->bor[u->outi]];
			if(dfn[v->id] == 0)
			{
				dfn[v->id] = low[v->id] = ++scc_index;
				scc_stk.push(v);
				in_stack[v->id] = true;

				scc_buff.push(u);
				u = v;
			}
			else
			{
				if(in_stack[v->id])
					low[u->id] = min(low[u->id], dfn[v->id]);
				u->outi--;
			}
		}
		if(dfn[u->id] == low[u->id])
		{
			SCC_Vtx *scc_u = new SCC_Vtx(scc_cnt++);
			scc_vtxs.push_back(scc_u);

			Vtx *v;
			do
			{
				v = scc_stk.top();
				scc_stk.pop();
				in_stack[v->id] = false;
				scc_u->scc_group.push_back(v);

				vtx_to_scc[v->id] = scc_u;	//record to toposort		

			}while(v != u);
		}
		u = scc_buff.top();
		scc_buff.pop();
		low[u->id] = min(low[u->id], low[vtxs[u->bor[u->outi]]->id]);
		u->outi--;
	}while(!scc_buff.empty() || u->outi >= 0 );
}


/*
 *	Topo sort
 */

//Topo sort global output
vector<SCC_Vtx*> topo_scc_vtxs;
vector<ID> scc_vtxs_map; // old to new

//Toposort que
queue<SCC_Vtx*> scc_que;
vector<SCC_Vtx*> scc_topo_buff[2];
queue<SCC_Vtx*> big_scc;


void add_scc_to_vec(SCC_Vtx *u, vector<SCC_Vtx*> &scc_topo_buff)
{
	if(u->scc_group.size() > BIG_SCC_SIZE)
		big_scc.push(u);
	else	scc_topo_buff.push_back(u);
}

void visit_scc_bor(SCC_Vtx *u, vector<SCC_Vtx*> &scc_topo_buff)
{
	for(ID i = 0; i < u->outcnt; i++)
	{
		SCC_Vtx *v = scc_vtxs[u->outbr[i]];
		v->incnt--;
		if(v->incnt == 0)
			add_scc_to_vec(v, scc_topo_buff);
	}
}

void copy_scc_topo_buff(vector<SCC_Vtx*> &scc_topo_buff, RawFile<ID> &scc_topo_level, cnt_t level)
{
	RawUnit<ID> &ru = scc_topo_level[level];
	ru.first = topo_scc_vtxs.size();
	for(cnt_t i = 0; i < scc_topo_buff.size(); i++)
	{
		topo_scc_vtxs.push_back(scc_topo_buff[i]);
		scc_vtxs_map[scc_topo_buff[i]->id] = topo_scc_vtxs.size() - 1;
	}
	ru.second = topo_scc_vtxs.size();
	//[from, to)
	
	cout << "scc level: " << ru.first << "-" << ru.second << endl;
}


/*
 *	RawFile<ID> scc_topo_level: for para calc small scc
 *	[from, to) = [from_scc_num, to_scc_num + 1);
 */
void build_toposort(RawFile<ID> &scc_topo_level) 
{
	init_topo_incnt();
	init_topo_sort();

	cnt_t level = 0;
	for (ID i = 0; i < scc_vtxs.size(); i++)
        {
		SCC_Vtx* u = scc_vtxs[i];
                if (u->incnt == 0)
                	add_scc_to_vec(u, scc_topo_buff[0]);
        }

	cnt_t itr = 0;
        while (!scc_topo_buff[0].empty() || !scc_topo_buff[1].empty() || !big_scc.empty())
        {
		//Calculate small scc.
		for(ID i = 0; i < scc_topo_buff[itr % 2].size(); i++)
		{
			SCC_Vtx *u = scc_topo_buff[itr % 2][i];
			visit_scc_bor(u, scc_topo_buff[(itr + 1) % 2]);
		}

		copy_scc_topo_buff(scc_topo_buff[itr % 2], scc_topo_level, level++);
		scc_topo_buff[itr % 2].clear();

		//Calculate big scc. Check if there are big scc
		while(!big_scc.empty())
		{
			SCC_Vtx *u = big_scc.front();
			big_scc.pop();
			visit_scc_bor(u, scc_topo_buff[(itr + 1) % 2]);

			topo_scc_vtxs.push_back(u);
			scc_vtxs_map[u->id] = topo_scc_vtxs.size() - 1;
			
			//[from, to)
			scc_topo_level[level].first = topo_scc_vtxs.size() - 1;
			scc_topo_level[level].second = topo_scc_vtxs.size();
			level++;
			cout << "big level: " << topo_scc_vtxs.size() -1 << endl;
		}		
		itr++;
        }
	scc_topo_level.set_raw_num(level);
	cout << "topo scc num:" << topo_scc_vtxs.size() << endl;
/*
	for(ID i = 0; i < topo_scc_vtxs.size(); i++)
	{
		SCC_Vtx* u = topo_scc_vtxs[i];

		if(u->scc_group.size() < 1)
			continue;

		cout << "scc-" << i;
		for(ID j = 0; j < u->scc_group.size(); j++)
			cout << " " << u->scc_group[j]->id;
		cout << endl;
	}
*/
}

void init_topo_incnt()
{
	for(ID i = 0; i < scc_vtxs.size(); i++)
	{
		SCC_Vtx* u = scc_vtxs[i];
		for(ID j = 0; j < u->outcnt; j++)
			scc_vtxs[u->outbr[j]]->const_incnt++;
	}
}

void init_topo_sort()
{
	for(ID i = 0; i < scc_vtxs.size(); i++)
		scc_vtxs[i]->incnt = scc_vtxs[i]->const_incnt;

	scc_vtxs_map.resize(scc_vtxs.size());
}

/*
 *	File operaton
 */

void read_dataset_adj_no_size(char *fname, RawFile<ID> &rfile)
{
	FILE *f = fopen(fname, "r");

	//Start read dataset
	cout << "Start read dataset "; get_time(false); cout << endl;
	ID from;
	long long maxlen = 10000000;
	char *s = (char*)malloc(maxlen);
	char delims[] = " \t\n";

	long long brcnt = 0;
	while (fgets(s, maxlen, f) != NULL)
	{
		if (s[0] == '#' || s[0] == '%' || s[0] == '\n')
			continue; //Comment

		char *t = strtok(s, delims);
		from = atoi(t);
		N = max(N, from);

		ID to;
		while ((t = strtok(NULL, delims)) != NULL)
		{
			to = atoi(t);
			N = max(N, to);
			
			RawUnit<ID> &ru = rfile[brcnt];
                        ru.first = from;
                        ru.second = to;
                        brcnt++;	
		}
		///*
		if (from % 100000 == 0)
			cout << from << endl;
		//*/
		/*
		if (++brcnt % 1000000 == 0)
		cout << brcnt << endl;
		//*/
	}
	rfile.set_raw_num(brcnt);
	fclose(f);
	free(s);
	cout << "End read dataset max id: " << N << " "; get_time(); cout << endl;
}


void read_dataset_hlist(char *fname, RawFile<ID> &rfile)
{
	read_dataset_adj_no_size(fname, rfile);
}

void convert_raw_to_adj(RawFile<ID> &raw_file, VtxsFile<ID> &adj_file, bool is_in_not_out = false, 
	ID* mapping = NULL)
{
	cnt_t raw_size = raw_file.size();

	ID last_from = (0 - 1); //define a non-exist id to last_from
	bool first_set = true;

	for(cnt_t i = 0; i < raw_size; i++)
	{
		RawUnit<ID> &ru = raw_file[i];
		ID from, to;
		
		//set from to
		if(!is_in_not_out)
		{
			from = ru.first;
			to = ru.second;
		}
		else
		{
			from = ru.second;
			to = ru.first;
		}
		
		if(mapping != NULL)
		{
			from = mapping[from];
			to = mapping[to];
		}

		if(last_from != from)
		{
			if(!first_set) 
				adj_file.add_vtx_end();
			else first_set = false;

			last_from = from;
			
			adj_file.add_vtx_start(from);
		}

		adj_file.add_bor(to);
	}
	if(!first_set)
		adj_file.add_vtx_end();
}

void build_topo_scc_file(VtxsFile<ID> &topo_fscc_vtxs)
{
	typedef set<SCC_Vtx*>::iterator sit;
	for(ID i = 0; i < topo_scc_vtxs.size(); i++)
	{
		SCC_Vtx *u = topo_scc_vtxs[i];
		topo_fscc_vtxs.add_vtx_start(i);
		for(int i = 0; i < u->outcnt; i++)
		{
			topo_fscc_vtxs.add_bor(scc_vtxs_map[u->outbr[i]]);
		}
		
		topo_fscc_vtxs.add_vtx_end();
	}

}

void build_vtxs_map(VtxIndexFile<ID> &vtxs_map_otn, VtxIndexFile<ID> &vtxs_map_nto) // vtxs_map[old] = new
{
	ID new_id = 0;
	for(ID i = 0; i < topo_scc_vtxs.size(); i++)
	{
		SCC_Vtx *u = topo_scc_vtxs[i];
		u->from_id = new_id;
		for(long long j =  u->scc_group.size() - 1; j >= 0; j--) // according to tarjan's dfs order for faster convergence
		{
			ID old_id = u->scc_group[j]->id;
			vtxs_map_otn[old_id] = new_id;		
			vtxs_map_nto[new_id++] = old_id;
		}
		u->to_id = new_id - 1;
	}
/*	
	cout << "otn map size: " << vtxs_map_otn.size() << endl;
	for(ID i = 0; i < vtxs_map_otn.size(); i++)
		cout << i << " -> " <<  vtxs_map_otn[i] << endl;
//*/
}

//build_order_vtxs_out compare function
int id_greater(const void *a, const void *b){return *(int*)a - *(int*)b;};
int id_less(const void *a, const void *b){return *(int*)b - *(int*)a;};

void sort_bor(VtxsFile<ID> &fvtxs, bool is_in_not_out)
{
	for(ID i = 0; i < fvtxs.size(); i++)
	{
		VtxUnit<ID> *vu = fvtxs[i];
		if(is_in_not_out) qsort((void*)vu->bor, *(vu->cnt_p), sizeof(ID), id_greater); //in is upper sort
		else qsort((void*)vu->bor, *(vu->cnt_p), sizeof(ID), id_less);	//out is lower sort
		delete vu;
	}
}

void build_order_vtxs_out(VtxsFile<ID> &vtxs_io, VtxIndexFile<ID> &vtxs_map_nto, VtxIndexFile<ID> &vtxs_map_otn)//, bool is_in_not_out)
{
	ID new_id = 0;
	for(new_id = 0; new_id < vtxs_map_nto.size(); new_id ++)
	{
		Vtx *v = vtxs[vtxs_map_nto[new_id]];
		vtxs_io.add_vtx_start(new_id);
		
		cnt_t bor_cnt = v->out_cnt;
		for(ID k = 0; k < bor_cnt; k++)
		{
			vtxs_io.add_bor(vtxs_map_otn[v->bor[k]]);
		}
		
		vtxs_io.add_vtx_end();

		//sort bor according to tarjan's dfs order for faster convergence
	/*	VtxUnit<ID> *vu = vtxs_io[new_id];
		if(is_in_not_out) qsort((void*)vu->bor, bor_cnt, sizeof(ID), id_greater); //in is upper sort
		else qsort((void*)vu->bor, bor_cnt, sizeof(ID), id_less);	//out is lower sort
		cout << "v-" << *(vu->id_p);
		for(ID k = 0; k < bor_cnt; k++)
			cout << " " << vu->bor[k];
		cout << endl;
		
		delete vu;
	*/
	}

	sort_bor(vtxs_io, false);
}

void build_scc_group_file(RawFile<ID> &fscc_group)
{
	for(ID i = 0; i < topo_scc_vtxs.size(); i++)
	{
		RawUnit<ID> &ru = fscc_group[i];
		SCC_Vtx *v = topo_scc_vtxs[i];
		ru.first = v->from_id;
		ru.second = v->to_id;
	}
	fscc_group.set_raw_num(topo_scc_vtxs.size());
/*
	for(ID i = 0; i < fscc_group.size(); i++)
	{
		RawUnit<ID> &ru = fscc_group[i];
		cout << "scc-" << i << " " << ru.first << "-" << ru.second << endl;
	}
*/
}

void convert_raw_file(RawFile<ID> &rfile, ID* mapping)
{
	#pragma omp parallel for schedule(static)
	for(cnt_t i = 0; i < rfile.size(); i++)
	{
		RawUnit<ID> &ru = rfile[i];
		ru.first = mapping[ru.first];
		ru.second = mapping[ru.second];
	}
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		cout << "Usage ./build <dataset.txt> <to dir>" << endl;
		return 0;
	}


	string dir(argv[2]);	

	//Remove old files.
	cout << "Remove old files"; get_time(false);
	string cmd = string("rm -f ") + dir + string("/*.bin");
	system(cmd.c_str());
	get_time(); cout << endl;

	//Record start time
	time_t start_time = get_time(false);

	cout << "\n------------------Read Dataset---------------------" << endl;
	//Create unsorted raw file from dataset
	cout << "Dataset --> Normal RawFile " << endl; get_time(false);
	RawFile<ID> us_rfile("unsorted_raw_file.bin", dir);
	us_rfile.open();
	read_dataset_adj_no_size(argv[1], us_rfile);
	cout << "Max id: " << N << endl;
	
	//Sort it
	cout << "Sort Normal RawFile --> Sorted  RawFile(first unit order) " << endl; get_time(false);
	RawFile<ID> s_rfile("sorted_raw_file.bin", dir);
	s_rfile.open();
	Sorter::sort_raw(us_rfile, s_rfile, true);
	get_time(); cout << endl;

	//Generate normal adj file
	cout << "Sorted RawFile(first unit order) --> Normal AdjFile "; get_time(false);
	VtxsFile<ID> fvtxs_out("normal_vtxs_out.bin", "normal_vindex_out.bin", dir);
	fvtxs_out.open();
	convert_raw_to_adj(s_rfile, fvtxs_out, false);
	get_time(); cout << endl;

	cout << "\n----------------Build SCC & Topo Sort--------------" << endl;
	//Build scc
	cout << "Build SCC " << endl; get_time(false);
	build_scc(fvtxs_out);
	get_time(); cout << endl;

	//Generate scc vtxs adj file
	cout << "Generate SCC --> SCC AdjFile "; get_time(false);
	VtxsFile<ID> fscc_vtxs("scc_vtx.bin", "scc_vindex.bin", dir);
	fscc_vtxs.open();
	build_scc_graph(fscc_vtxs);
	get_time(); cout << endl;

	//Build topo sort
	cout << "Topo Sort --> SCC Topo Level File" << endl; get_time(false);
	RawFile<ID> scc_topo_level("scc_topo_level.bin", dir);
	scc_topo_level.open();
	build_toposort(scc_topo_level);	
	scc_topo_level.close();
	get_time(); cout << endl;
/*
	//Generate scc vtxs adj file
	cout << "Generate Topo SCC --> Topo SCC AdjFile "; get_time(false);
	VtxsFile<ID> topo_fscc_vtxs("topo_scc_vtx.bin", "topo_scc_vindex.bin", dir);
	topo_fscc_vtxs.open();
	build_topo_scc_file(topo_fscc_vtxs);
	topo_fscc_vtxs.close();
	get_time(); cout << endl;
//*/
	//Generate vtx map file
	cout << "Convert Vertex ID --> Vertex MapFile "; get_time(false);
	VtxIndexFile<ID> vtxs_map_otn("vtxs_map_old_to_new.bin", dir), vtxs_map_nto("vtxs_map_new_to_old.bin", dir);
	vtxs_map_otn.open();
	vtxs_map_nto.open();
	build_vtxs_map(vtxs_map_otn, vtxs_map_nto);
	get_time(); cout << endl;

	cout << "\n---------------Generate Out AdjFile----------------" << endl;
	//Generate order vtx outbor file
	cout << "Generate Vertex Out AdjFile "; get_time(false);
	VtxsFile<ID> vtxs_out("order_vtxs_out.bin", "order_vindex_out.bin", dir);
	vtxs_out.open();
	build_order_vtxs_out(vtxs_out, vtxs_map_nto, vtxs_map_otn);//, false); //out	
	//vtxs_map_nto.close();
	//vtxs_out.close();
	get_time(); cout << endl;

	cout << "\n---------------Generate In AdjFile-----------------" << endl;
	//Generate order vtx inbor file
	cout << "Convert Normal RawFile ID --> New ID RawFile "; get_time(false);
	convert_raw_file(us_rfile, vtxs_map_otn.get_start());
	get_time(); cout << endl;

	cout << "Sort New ID RawFile --> Sorted RawFile(second unit order) " << endl; get_time(false);
	Sorter::sort_raw(us_rfile, s_rfile, false);	
	get_time(); cout << endl;

	cout << "Sorted RawFile(second unit order) --> Vertex In AdjFile "; get_time(false);
	VtxsFile<ID> vtxs_in("order_vtxs_in.bin", "order_vindex_in.bin", dir);
	vtxs_in.open();
	convert_raw_to_adj(s_rfile, vtxs_in, true);
	sort_bor(vtxs_in, true);
	//vtxs_in.close();
	get_time(); cout << endl;

	//vtxs_map_otn.close();
	us_rfile.close();
	s_rfile.close();

	cout << "\n--------------Generate SCC Group File--------------" << endl;
	//Generate scc group file
	cout << "Generate SCC Group File "; get_time(false);
	RawFile<ID> fscc_group("scc_group.bin", dir);
	fscc_group.open();
	build_scc_group_file(fscc_group);	
	fscc_group.close();
	//fvtxs_out.close();
	get_time(); cout << endl;

	cout << "\n--------------Generate PathGraph File--------------" << endl;
	//Generate PathGraph File in SCC DAg order
	cout << "Generate SCC Group PathGraph File "; get_time(false);
	//Matrixso map
	cout << endl << "Building Matrixso Map" << endl; 
	VtxIndexFile<ID> pg_map_so_otn("pg_map_matrixso_otn.bin", dir), pg_map_so_nto("pg_map_matrixso_nto.bin", dir);
	pg_map_so_otn.open();
	pg_map_so_nto.open();
	PathGraph::generate_map(topo_scc_vtxs, vtxs_out, vtxs_map_otn, pg_map_so_otn, pg_map_so_nto, false, vtxs_map_nto.get_start());
	pg_map_so_otn.close();
	pg_map_so_nto.close();
	get_time(); cout << endl;

	//Matrixos map
	cout << "Building Matrixos Map" << endl; 
	VtxIndexFile<ID> pg_map_os_otn("pg_map_matrixos_otn.bin", dir), pg_map_os_nto("pg_map_matrixos_nto.bin", dir);
	pg_map_os_otn.open();
	pg_map_os_nto.open();
	//PathGraph::generate_map(topo_scc_vtxs, vtxs_in, vtxs_map_otn, pg_map_os_otn, pg_map_os_nto, true, vtxs_map_nto.get_start());
	PathGraph::generate_map_os(topo_scc_vtxs, vtxs_in, vtxs_map_otn, pg_map_os_otn, pg_map_os_nto, N, vtxs_map_nto.get_start());
	pg_map_os_nto.close();
	get_time(); cout << endl;

	//Close scc vtxs data
	fscc_vtxs.close();
	//Convert adj to Matrixos adj file
	cout << "Convert adj file to Matrixos adj File" << endl;

	/*
 	*	To close PathGraph: add comment "//" to 2 lines below and modify api.h map_nto & map_otn
 	*	Re-make the project and re-build the dataset.
 	* */
	PathGraph::convert_adj_file(vtxs_out, pg_map_os_otn, vtxs_map_nto.get_start());
	PathGraph::convert_adj_file(vtxs_in, pg_map_os_otn, vtxs_map_nto.get_start());
	pg_map_os_otn.close();
	vtxs_map_nto.close();

	cout << "Generate compress PathGraph vtxs file" << endl;
	PGVtxsFile<ID> pg_vtxs_out("pg_vtxs_out.bin", "pg_vindex_out.bin", dir);
	pg_vtxs_out.open();
	pg_vtxs_out.generate(vtxs_out);
	pg_vtxs_out.close();
	vtxs_out.close();

	PGVtxsFile<ID> pg_vtxs_in("pg_vtxs_in.bin", "pg_vindex_in.bin", dir);
	pg_vtxs_in.open();
	pg_vtxs_in.generate(vtxs_in);
	pg_vtxs_in.close();
	vtxs_in.close();

	vtxs_map_otn.close();
	fvtxs_out.close();

	get_time(); cout << endl;

	cout << "---------------------------------------------------" << endl;
	cout << "Build end." << " cost: " << get_time(false) - start_time << " ms" << endl;
	return 0;
}

