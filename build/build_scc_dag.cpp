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


/*
 cnt_t scc_size = scc_vtxs.size();
	cout << "scc num: " << scc_size << endl;
	for(ID i = 0; i < scc_size; i++)
	{
		SCC_Vtx *scc = scc_vtxs[i];
		cout << "scc-" << scc->id << " : ";
		for(ID j = 0; j < scc->scc_group.size(); j++)
			cout << scc->scc_group[j]->id << " " ;
		cout << endl;
	}
//*/
	//Make scc graph
	cout << "scc num: " << scc_vtxs.size() << endl;

	//Free
	dfn.clear();
	low.clear();
	in_stack.clear();
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
			scc_u->scc_group_id.push_back(scc_u->scc_group[j]->id); //store to id vector to narrow space
		}
		scc_u->scc_group.clear();

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
	//Delete vtx to narrow space 
	for(ID i = 0; i < N; i++)
		delete vtxs[i];
	vtxs.clear();
	vtx_to_scc.clear();
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
	if(u->scc_group_id.size() > BIG_SCC_SIZE)
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
	//bool is_first_print = true; //Print toposort's top vtx
	for (ID i = 0; i < scc_vtxs.size(); i++)
        {
		SCC_Vtx* u = scc_vtxs[i];
                if (u->incnt == 0)
                	add_scc_to_vec(u, scc_topo_buff[0]);
		//Print toposort's top vtx
		//if(is_first_print && u->incnt == 0 && (u->outcnt != 0 || u->scc_group.size() > BIG_SCC_SIZE)) //only for dblp
		/*
		if(u->incnt == 0 && u->outcnt > 10 && u->outcnt < 30)
		{
			cout << "Top vtx: " << u->scc_group[0]->id << " out size: " << u->outcnt << endl;
			//is_first_print = false;
		}
		*/
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

void convert_raw_to_adj(RawFile<ID> &raw_file, VtxsFile<ID> &adj_file, bool is_in_not_out = false, 
	ID* mapping = NULL)
{
	cnt_t raw_size = raw_file.size();

	cout << "raw_size" << raw_size << endl;

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

void convert_id_to_vid(vector<SCC_Vtx*> &topo_scc_vtxs, VtxIndexFile<ID> &map)
{
	for(cnt_t i = 0; i < topo_scc_vtxs.size(); i++)
	{
		vector<ID> &group_id = topo_scc_vtxs[i]->scc_group_id;
		for(cnt_t j = 0; j < group_id.size(); j++)
			group_id[j] = map[group_id[j]];
		sort(group_id.begin(), group_id.end()); //increase order
	}

}


void generate_vid_order(VtxIndexFile<ID> &vid_map,vector<SCC_Vtx*> &topo_scc_vtxs)
{
	ID index = 0;
	for(cnt_t i = 0; i < topo_scc_vtxs.size(); i++)
	{
		vector<ID> &group_id = topo_scc_vtxs[i]->scc_group_id;
		for(cnt_t j = 0; j < group_id.size(); j++)
		{
			vid_map[index++] = group_id[j];
		}
	}

	//Print order
	//for(cnt_t i = 0; i < vid_map.size(); i++)
		//cout << i << "-" << vid_map[i] << endl;
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

void build_scc_group_file(RawFile<ID> &fscc_group)
{
	//Group file Raw: [from_id, to_id], [from_id, to_id] (use [] not (), include edge number)
	ID start_id = 0;
	for(ID i = 0; i < topo_scc_vtxs.size(); i++)
	{
		RawUnit<ID> &ru = fscc_group[i];
		SCC_Vtx *v = topo_scc_vtxs[i];
		ru.first = start_id;
		start_id += v->scc_group_id.size();
		ru.second = start_id - 1; //to id  
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


int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		cout << "Usage ./build_scc_dag <dir>" << endl;
		return 0;
	}


	string dir(argv[1]);	

	time_t start_time = get_time(false);

	cout << "\n----------------Build SCC & Topo Sort--------------" << endl;
	//Open normal adj file
	VtxsFile<ID> fvtxs_out("normal_vtxs_out.bin", "normal_vindex_out.bin", dir);
	fvtxs_out.open();
	N = fvtxs_out.size() - 1; // get max id

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
	fvtxs_out.close();

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


	cout << "\n--------------Generate SCC Group File--------------" << endl;
	//Generate scc group file
	cout << "Generate SCC Group File "; get_time(false);
	RawFile<ID> fscc_group("scc_group.bin", dir);
	fscc_group.open();
	build_scc_group_file(fscc_group);	
	fscc_group.close();
	fscc_vtxs.close();
	fscc_vtxs.destroy(); //SCC graph doesn't need at all. And we doesn't need to export scc belongs to which level. 
	get_time(); cout << endl;

	cout << "\n--------------Generate PathGraph VID File--------------" << endl;
	//Generate PathGraph File in SCC DAG order
	cout << "Building Matrixos Map" << endl; 
	VtxIndexFile<ID> pg_map_os_otn("pg_id_to_vid.bin", dir), pg_map_os_nto("pg_vid_to_id.bin", dir);
	pg_map_os_otn.open();
	pg_map_os_nto.open();
	VtxsFile<ID> fvtxs_in("normal_vtxs_in.bin", "normal_vindex_in.bin", dir);
	fvtxs_in.open();
	PathGraph::generate_pg_map(topo_scc_vtxs, fvtxs_in, pg_map_os_otn, pg_map_os_nto, N);
	fvtxs_in.close();
	get_time(); cout << endl;

	cout << "Convert ID to VID " << endl;
	convert_id_to_vid(topo_scc_vtxs, pg_map_os_otn);
	get_time(); cout << endl;

	cout << "Generate VID Order" << endl;
	VtxIndexFile<ID> vid_map("calc_vid_order.bin", dir);
	vid_map.open();
	generate_vid_order(vid_map, topo_scc_vtxs);
	get_time(); cout << endl;

	cout << "\n--------------Generate PathGraph vtxs File--------------" << endl;
	cout << "Generate in file " << endl;
	OutPGVtxsFile pg_vtxs_in("pg_vtxs_in.bin", dir);
	pg_vtxs_in.open();
	fvtxs_in.open();
	pg_vtxs_in.generate(vid_map, pg_map_os_otn, pg_map_os_nto,  fvtxs_in);
	pg_vtxs_in.close();
	fvtxs_in.close();
	fvtxs_in.destroy();
	get_time(); cout << endl;


	cout << "Generate out file " << endl;
	OutPGVtxsFile pg_vtxs_out("pg_vtxs_out.bin", dir);
	pg_vtxs_out.open();
	fvtxs_out.open();
	pg_vtxs_out.generate(vid_map, pg_map_os_otn, pg_map_os_nto, fvtxs_out);
	pg_vtxs_out.close();
	fvtxs_out.close();
	fvtxs_out.destroy();
	get_time(); cout << endl;


	vid_map.close();
	pg_map_os_otn.close();
	pg_map_os_nto.close();


	

/*
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
	//fvtxs_out.close();

	get_time(); cout << endl;
*/
	cout << "---------------------------------------------------" << endl;
	cout << "Build SCC-DAG finish. Expires: " << get_time(false) - start_time << " ####" << endl;
	return 0;
}

