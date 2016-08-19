#ifndef SCC_DAG_API_H
#define SCC_DAG_API_H

#include "type.h"
#include "gfile.h"
#include "pathgraph.h"
#include "debug.h"
#include <string>
#include <atomic>
#include <cstdlib>
#include <omp.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <xmmintrin.h>

/*
class ApiFileName
{
public:
	static const string 
		normal_vindex_out("normal_vindex_out.bin"),
		normal_vtx("normal_vtx.bin"),
		
		order_vindex_out("order_vindex_out.bin"),
		order_vtxs_out("order_vtxs_out.bin"),
		
		order_vindex_in("order_vindex_in.bin"),
		order_vtxs_in("order_vtxs_in.bin"),

		scc_vindex("scc_vindex.bin"),
		scc_vtx("scc_vtx.bin"),

		scc_group("scc_group.bin"),

		vtxs_map_new_to_old("vtxs_map_new_to_old"),
		vtxs_map_old_to_new("vtxs_map_old_to_new");
}
*/

class Vtx
{
public:
        ID virtual_id;
        bor_cnt_t cnt;
        ID* bor;
        Vtx(VtxUnit<ID>* vu): 
			virtual_id(*(vu->id_p)),
                        cnt(*(vu->cnt_p)),
			bor(vu->bor){delete vu;};
};



class SCC_Vtx
{
public:
	ID id, from_id, to_id;
	bor_cnt_t out_cnt; //we only construct scc vtx out file.
	ID* bor;
	SCC_Vtx(VtxUnit<ID> *vu, RawUnit<ID> &ru): 
			id(*(vu->id_p)),
                        out_cnt(*(vu->cnt_p)), 
			bor(vu->bor),
			from_id(ru.first),
			to_id(ru.second)
			{delete vu;};
};

template <class T>
class PreLoadSccDagApp
{
protected:
	VtxsFile<ID> fvtxs_out, fvtxs_in, fscc_vtx;
	RawFile<ID> fscc_group, fscc_topo_level;
	VtxIndexFile<ID> map_nto, map_otn;

	cnt_t vtxs_size, scc_size;

	vector<Vtx*> vtxs_in, vtxs_out;// vtxs_out; out is not used at all
	vector<SCC_Vtx*> scc_vtxs;

	//typedef VtxIndexFile<T> values_t;
	typedef T* values_t;
	values_t value1, value2, value1_bak, value2_bak;

	void swap_value(values_t &a, values_t &b){values_t t = a; a = b; b = t;}

public:
	PreLoadSccDagApp(string dir, string value_name, string value_dir): 
		fvtxs_out("order_vtxs_out.bin", "order_vindex_out.bin", dir),
		fvtxs_in("order_vtxs_in.bin", "order_vindex_in.bin", dir),
		fscc_vtx("scc_vtx.bin", "scc_vindex.bin", dir),
		fscc_group("scc_group.bin", dir),
		fscc_topo_level("scc_topo_level.bin", dir),
		map_nto("pg_map_matrixos_nto.bin", dir),// PathGraph map
		map_otn("pg_map_matrixos_otn.bin", dir)//, PathGraph map
		//map_nto("vtxs_map_new_to_old.bin", dir), //no PathGraph map
		//map_otn("vtxs_map_old_to_new.bin", dir)  //no PathGraph map
		//value(value_name, value_dir)
		{
			cout << "Loading SCC DAG data..." << endl;
			fvtxs_out.open();
			fvtxs_in.open();
			fscc_vtx.open();
			fscc_group.open();
			fscc_topo_level.open();
			map_nto.open();
			map_otn.open();
				
			//initral vtxs
			cout << "initral vtxs data" << endl;
			vtxs_size = max(fvtxs_in.size(), fvtxs_out.size());
			vtxs_in.resize(vtxs_size);
			for(ID i = 0; i < vtxs_size; i++)
				vtxs_in[i] = new Vtx(fvtxs_in[i]);
			vtxs_out.resize(vtxs_size);
			for(ID i = 0; i < vtxs_size; i++)
				vtxs_out[i] = new Vtx(fvtxs_out[i]);
			
			//initral value;
			cout << "initral value container" << endl;
			//value.open();
			value1 = value1_bak = (T*)malloc(vtxs_size * sizeof(T));
			value2 = value2_bak = (T*)malloc(vtxs_size * sizeof(T));

			//initral scc_vtxs
			cout << "initral scc data" << endl;
			scc_size = fscc_vtx.size();
			scc_vtxs.resize(scc_size);
			for(ID i = 0; i < scc_size; i++)
				scc_vtxs[i] = new SCC_Vtx(fscc_vtx[i], fscc_group[i]);
			cout << "Load success." << endl;
		};

	~PreLoadSccDagApp()
	{
		for(int i = 0; i < vtxs_size; i++)
		{
			delete vtxs_in[i];		
			delete vtxs_out[i];
		}

		for(int i = 0; i < scc_size; i++)
			delete scc_vtxs[i];

		fvtxs_out.close();
		fvtxs_in.close();
		fscc_vtx.close();
		fscc_group.close();
		fscc_topo_level.close();
		map_nto.close();
		map_otn.close();

		//delete value
		//value.close();
		free(value1_bak);
		free(value2_bak);
	}

	virtual void init(ID itr, values_t &value) = 0;

/*
 *	Notice: Sometime id != vtx_in[id]->virtual_id, for algorithm requirement.
 *	To get vtx value, using value[ vtx_in[id]->virtual_id ].
 */
	virtual bool update(ID itr, values_t &rvalue, values_t &wvalue) = 0; 

	void serial_normal_run()
	{
		//reset();
		cnt_t cvg_cnt = 0, size = vtxs_size;
		do
		{
			cvg_cnt = 0;
			for(ID i = 0; i < size; i++)
				cvg_cnt += update(i, value1, value2);
			swap_value(value1, value2);
		}while(cvg_cnt != size);	
	}	


	void serial_run()
	{
		//reset();
		cnt_t cvg_cnt = 0, size = 0;
		for(ID i = 0; i < scc_size; i++)
		{
			SCC_Vtx *v = scc_vtxs[i];
			size = v->to_id - v->from_id + 1;
			do
			{
				cvg_cnt = 0;
				for(ID j = v->from_id; j <= v->to_id; j++)
					cvg_cnt += update(j, value1, value2);
				swap_value(value1, value2);
			}while(cvg_cnt != size);
		}
	}

	void para_normal_run2() //Old version, not used at all
	{
		//reset();
		
		bool cvg = true;	
		cnt_t size = vtxs_size;
		do
		{
			cvg = true;
			//Normal run strategy divides the partition to equal distribution.
			#pragma omp parallel for schedule(static)
			for(ID i = 0; i < size; i++)
			{
				if(update(i, value1, value2) == false) 
					cvg = false;
			}
			swap_value(value1, value2);
		}while(!cvg);	
	}

	void para_normal_run()
	{
		//reset();
		
		int cvg = 0;
		int tnum = omp_get_max_threads();
		vector<int> cvgs(tnum, 1);	
		cnt_t size = vtxs_size;
		do
		{
			//Normal run strategy divides the partition to equal distribution.
			#pragma omp parallel for schedule(static) 
			for(ID i = 0; i < size; i++)
			{
				if(update(i, value1, value2) == false) 
					_mm_stream_si32(&(cvgs[omp_get_thread_num()]), 0);
			}
			swap_value(value1, value2);

			cvg = 0;
			for(int i = 0; i < tnum; i++)
			{
				cvg += cvgs[i];
				_mm_stream_si32(&(cvgs[i]), 1);
			}
		}while(cvg != tnum);	
	}

	void para_scc_run() //Old version, not used at all
	{
		//reset();
		bool cvg = true;
		cnt_t size = 0;
		for(ID i = 0; i < scc_size; i++)
		{
			SCC_Vtx *v = scc_vtxs[i];
			size = v->to_id - v->from_id + 1;
			cvg = true;
			do
			{
				cvg = true;
				if(size < 1024)
				{
					for(ID j = v->from_id; j <= v->to_id; j++)
					{
						if(update(j, value1, value2) == false)
							cvg = false;
					}
				}
				else
				{
					#pragma omp parallel for schedule(static, 500)
					for(ID j = v->from_id; j <= v->to_id; j++)
					{
						if(update(j, value1, value2) == false)
							cvg = false;
					}
				}
				swap_value(value1, value2);	
			}while(!cvg);
		}
	}

	void para_run()
	{
		//cout << endl << "start reset" << endl; get_time(false);
		//reset();
		//get_time();
		cout << endl;
		for(cnt_t i = 0; i < fscc_topo_level.size(); i++)
		{
			RawUnit<ID> &ru = fscc_topo_level[i];
			//cout << "calc scc " << ru.first << "-" << ru.second << " ";
			if(ru.first == ru.second - 1) //big scc
			{
				SCC_Vtx *v = scc_vtxs[ru.first];
				cnt_t size = v->to_id - v->from_id + 1, itr_cnt = 0;
				///*
				int cvg = 0;
				int tnum = omp_get_max_threads();
				vector<int> cvgs(tnum, 1); 
				do
				{
					/*
 					*	Use schedule(static, x) to calc scc.
					*	x = 500 is according to the L3 cache size and the number of cpu core. 
					*	x is too small : L1 L2 cache miss rate high
					*	x is too big : L3 cache miss rate high
 					*/ 
					#pragma omp parallel for schedule(static, 500)
					for(ID j = v->from_id; j <= v->to_id; j++)
					{
						if(update(j, value1, value2) == false)
							_mm_stream_si32(&(cvgs[omp_get_thread_num()]), 0);    //SSE instructon bypassing cache
					}
					swap_value(value1, value2);
					cvg = 0;
					for(int j = 0; j < tnum; j++)
					{
						cvg += cvgs[j];
						_mm_stream_si32(&(cvgs[j]), 1);
					}
					//schedule(static, x)
					itr_cnt++;
				}while(cvg != tnum);
				cout << "size: " << size << " cvg at: " << itr_cnt << endl;
			}
			else //small scc
			{
				#pragma omp parallel for schedule(dynamic, 1000) //1000 is experience
				for(ID i = ru.first; i < ru.second; i++)
				{
					SCC_Vtx *v = scc_vtxs[i];
					cnt_t size = v->to_id - v->from_id + 1, cvg_cnt = 0;
					do
					{
						cvg_cnt = 0;
						for(ID j = v->from_id; j <= v->to_id; j++)
							cvg_cnt += update(j, value1, value2);
						swap_value(value1, value2);
					}while(size != cvg_cnt);
				}
			}
			//get_time(); cout << endl; 
		}

	}

        void print_top(int num)
        {
                int gap = vtxs_size / num;
                cout << "Top value:" << endl;
                for (int i = 0; i < num; i++)
                        cout << i * gap << ":\t" << value2[map_otn[i * gap]] << endl;
                cout << endl;
        }

	void reset(bool set_is_syn_not_asyn = true)
	{
		if(set_is_syn_not_asyn)
		{
			value1 = value1_bak;
			value2 = value2_bak;
		}
		else
			value1 = value2 = value1_bak;

		for(ID i = 0; i < vtxs_size; i++)
		{
			init(i, value1);
			init(i, value2);
		}
	}

private:
	template <class MAXT>
	MAXT max(MAXT a, MAXT b){return a < b ? b : a;}
};

template <class T>
class UnSccDagApp
{
protected:
	VtxsFile<ID> fvtxs_out, fvtxs_in;

	cnt_t vtxs_size;

	//typedef VtxIndexFile<T> values_t;
	typedef T value_t;
	typedef T* values_t;
	typedef ID* bor_t;
	values_t value1, value2, value1_bak, value2_bak;

	void swap_value(values_t &a, values_t &b){values_t t = a; a = b; b = t;}
	/*void swap_value(values_t &a, values_t &b, cnt_t from = 0, cnt_t to = vtxs_size - 1)
	{
		if(a == b)
			return;
		#pragma parallel for
		for(cnt_t i = from; i <= to; i++)
			a[i] = b[i];
	}*/

public:
	UnSccDagApp(string dir, string value_name, string value_dir): 
		fvtxs_out("normal_vtxs_out.bin", "normal_vindex_out.bin", dir),
		fvtxs_in("normal_vtxs_in.bin", "normal_vindex_in.bin", dir)
		//value(value_name, value_dir)
		{
			cout << "Loading normal data..." << endl;
			fvtxs_out.open();
			fvtxs_in.open();
			if(fvtxs_in.size() == 0)
			{
				cout << "Error: Match group doesn't exist. Run: build_match_group before hand." << endl;
				exit(-1);
			}
			//initral vtxs
			vtxs_size = max(fvtxs_in.size(), fvtxs_out.size());
			
			//initral value;
			//value.open();
			value1 = value1_bak = (T*)malloc(vtxs_size * sizeof(T));
			value2 = value2_bak = (T*)malloc(vtxs_size * sizeof(T));

			cout << "Load success." << endl;
		};

	~UnSccDagApp()
	{
		fvtxs_out.close();
		fvtxs_in.close();

		//delete value
		//value.close();
		free(value1_bak);
		free(value2_bak);
	}

/*
 *	Notice: Normal data virtual_id == id
 */
	virtual void init(ID id, value_t &value) = 0;
	virtual bool update(ID virtual_id, ID* inbor, ID* outbor, cnt_t in_cnt, cnt_t out_cnt,
				 const values_t rvalue, values_t wvalue) = 0; 

	void serial_run()
	{
		//reset();
		cnt_t cvg_cnt = 0, size = vtxs_size, iter_times = 0;
		do
		{
			cvg_cnt = 0;
			for(ID i = 0; i < size; i++)
			{
				addr_t in_base = fvtxs_in.get_vtx_base_addr(i),
				     out_base = fvtxs_out.get_vtx_base_addr(i);

				cvg_cnt += update(
					map_otn[i],
					VtxUnitIdData::bor(in_base),
					VtxUnitIdData::bor(out_base),
					VtxUnitIdData::bor_cnt(in_base),
					VtxUnitIdData::bor_cnt(out_base),
					value1, value2);
			}
			swap_value(value1, value2);
			iter_times++;
			cout << cvg_cnt << ":" << vtxs_size << endl;
		}while(cvg_cnt != size);	
		cout << "itera times: " << iter_times << endl;
	}	

	void para_run()
	{
		//reset();
		
		int cvg = 0, iter_times = 0;
		int tnum = omp_get_max_threads();
		vector<int> cvgs(tnum, 1);	
		cnt_t size = vtxs_size;
		do
		{
			//Normal run strategy divides the partition to equal distribution.
			addr_t in_base, out_base;
			#pragma omp parallel for schedule(static) private(in_base, out_base)
			for(ID i = 0; i < size; i++)
			{
				in_base = fvtxs_in.get_vtx_base_addr(i);
				out_base = fvtxs_out.get_vtx_base_addr(i);

				if(update(
					map_otn[i],
					VtxUnitIdData::bor(in_base),
					VtxUnitIdData::bor(out_base),
					VtxUnitIdData::bor_cnt(in_base),
					VtxUnitIdData::bor_cnt(out_base),
					value1, value2) == false) 
					_mm_stream_si32(&(cvgs[omp_get_thread_num()]), 0);
			}
			swap_value(value1, value2);
			cvg = 0;
			for(int i = 0; i < tnum; i++)
			{
				cvg += cvgs[i];
				_mm_stream_si32(&(cvgs[i]), 1);
			}
			iter_times++;
		}while(cvg != tnum);
		cout << "itera times: " << iter_times << endl;	
	}

    void print_top(int num)
    {
            int gap = vtxs_size / num;
            cout << "Top value:" << endl;
            for (int i = 0; i < num; i++)
                    cout << i * gap << ":\t" << value2[i * gap] << endl;
            cout << endl;
    }

	void reset(bool set_is_syn_not_asyn = true)
	{
		if(set_is_syn_not_asyn)
		{
			value1 = value1_bak;
			value2 = value2_bak;
		}
		else
			value1 = value2 = value1_bak;

		for(ID i = 0; i < vtxs_size; i++)
		{
			ID id =  map_nto[i];
			init(id, value1[i]);
			init(id, value2[i]);
		}
	}

	//Normal map are the same id	
	template <class MT>
	class UnSccDagMap
	{
	public:
		MT operator[](MT id){return id;}
	};

	UnSccDagMap<ID> map_nto, map_otn;

private:
	template <class MAXT>
	MAXT max(MAXT a, MAXT b){return a < b ? b : a;}
};



typedef struct SccGroupUnit
{
	ID from_id, to_id;
}SccGroupUnit;

template <class T>
class SccDagApp
{
protected:
	//PGVtxsFile<ID> fvtxs_out, fvtxs_in;
	VtxsFile<ID> fvtxs_out, fvtxs_in;
	VtxsFile<ID> fscc_vtx;
	RawFile<ID> fscc_group, fscc_topo_level;
	VtxIndexFile<ID> map_nto, map_otn;//, dfs_map_nto, dfs_map_otn;

	cnt_t vtxs_size, scc_size;


	//typedef VtxIndexFile<T> values_t;
	typedef T value_t;
	typedef T* values_t;
	typedef ID* bor_t;
	//typedef PGVtxBor_t<ID> bor_t;
	values_t value1, value2, value1_bak, value2_bak;

	void swap_value(values_t &a, values_t &b){values_t t = a; a = b; b = t;}

public:
	SccDagApp(string dir, string value_name, string value_dir): 
		fvtxs_out("order_vtxs_out.bin", "order_vindex_out.bin", dir),
		fvtxs_in("order_vtxs_in.bin", "order_vindex_in.bin", dir),
	//	fvtxs_out("pg_vtxs_out.bin", "pg_vindex_out.bin", dir),
	//	fvtxs_in("pg_vtxs_in.bin", "pg_vindex_in.bin", dir),
		fscc_vtx("scc_vtx.bin", "scc_vindex.bin", dir),
		fscc_group("scc_group.bin", dir),
		fscc_topo_level("scc_topo_level.bin", dir),
		map_nto("pg_map_matrixos_nto.bin", dir),// PathGraph map
		map_otn("pg_map_matrixos_otn.bin", dir)//,// PathGraph map

		//dfs_map_nto("vtxs_map_new_to_old.bin", dir), //no PathGraph map
		//dfs_map_otn("vtxs_map_old_to_new.bin", dir)  //no PathGraph map
		//value(value_name, value_dir)
		{
			cout << "Loading SCC DAG data..." << endl;
			fvtxs_out.open();
			fvtxs_in.open();
			fscc_vtx.open();
			fscc_group.open();
			fscc_topo_level.open();
			map_nto.open();
			map_otn.open();
			//dfs_map_nto.open();
			//dfs_map_otn.open();
		
			//initral vtxs
			cout << "initral vtxs data" << endl;
			vtxs_size = max(fvtxs_in.size(), fvtxs_out.size());

			//initral value;
			cout << "initral value container" << endl;
			//value.open();
			value1 = value1_bak = (T*)malloc(vtxs_size * sizeof(T));
			value2 = value2_bak = (T*)malloc(vtxs_size * sizeof(T));

			//initral scc_vtxs
			cout << "initral scc data" << endl;
			scc_size = fscc_vtx.size();
			cout << "Load success." << endl;
		};

	~SccDagApp()
	{
		fvtxs_out.close();
		fvtxs_in.close();
		fscc_vtx.close();
		fscc_group.close();
		fscc_topo_level.close();
		map_nto.close();
		map_otn.close();
		//dfs_map_nto.close();
		//dfs_map_otn.close();

		//delete value
		//value.close();
		free(value1_bak);
		free(value2_bak);
	}


/*
 * 	API: init() and update()
 * 	You need to instantiate them in your own class(Consult in demo.cpp)
 *	Notice: Sometimes, virtual_id is not the original id in dataset for algorithm(PathGraph) requirement.
 *	To get id value, using map_nto[ virtual_id ].
 */
	virtual void init(ID id, value_t &value) = 0;
	virtual bool update(ID virtual_id, bor_t inbor, bor_t outbor, cnt_t in_cnt, cnt_t out_cnt,
				 const values_t rvalue, values_t wvalue) = 0; 

	void serial_run()
	{
		//reset();
		cnt_t cvg_cnt = 0, size = 0;
		for(ID i = 0; i < scc_size; i++)
		{
			SccGroupUnit &scc_group = (SccGroupUnit&)fscc_group[i];
			size = scc_group.to_id - scc_group.from_id + 1;
			do
			{
				cvg_cnt = 0;
				for(ID j = scc_group.from_id; j <= scc_group.to_id; j++)
				{
					addr_t in_base = fvtxs_in.get_vtx_base_addr(j),
						   out_base = fvtxs_out.get_vtx_base_addr(j);

					cvg_cnt += update(
							VtxUnitIdData::id(in_base == NULL ? out_base : in_base),
							VtxUnitIdData::bor(in_base),
							VtxUnitIdData::bor(out_base),
							VtxUnitIdData::bor_cnt(in_base),
							VtxUnitIdData::bor_cnt(out_base),
							value1, value2);
				}
				swap_value(value1, value2);
			}while(cvg_cnt != size);
		}
	}

	void para_run()
	{
		//cout << endl << "start reset" << endl; get_time(false);
		//reset();
		//get_time();
		cout << endl;
		for(cnt_t i = 0; i < fscc_topo_level.size(); i++)
		{
			//Get a scc level
			RawUnit<ID> &ru = fscc_topo_level[i];

			//cout << "calc scc " << ru.first << "-" << ru.second << " size: " << ru.second - ru.first << endl ;
			if(ru.first == ru.second - 1) //a big scc
			{
				SccGroupUnit &scc_group = (SccGroupUnit&)fscc_group[ru.first];
				cnt_t size = scc_group.to_id - scc_group.from_id + 1, itr_cnt = 0;

				int cvg = 0;
				int tnum = omp_get_max_threads();
				vector<int> cvgs(tnum, 1); 
				//vector<bor_t> inbors(tnum), outbors(tnum);
				do
				{
					/*
					 *	Use schedule(static, x) to calc scc.
					 *	x = 500 is according to the L3 cache size and the number of cpu core. 
					 *	x is too small : L1 L2 cache miss rate high
					 *	x is too big : L3 cache miss rate high
					 */
					addr_t in_base, out_base; //global parameter is faster(set private arguement)
#pragma omp parallel for schedule(static, 500) private(in_base, out_base)
					for(ID j = scc_group.from_id; j <= scc_group.to_id; j++)
					{
						in_base = fvtxs_in.get_vtx_base_addr(j);
						out_base = fvtxs_out.get_vtx_base_addr(j); 
						//out_base will never be null, we construct it for full vertix data

						if(update( 
									VtxUnitIdData::id(out_base == NULL? in_base : out_base),
									VtxUnitIdData::bor(in_base),
									VtxUnitIdData::bor(out_base),
									VtxUnitIdData::bor_cnt(in_base),
									VtxUnitIdData::bor_cnt(out_base),
									value1, value2) == false)
							_mm_stream_si32(&(cvgs[omp_get_thread_num()]), 0);  //SSE instruction 

						/*	
						 *	Use SSE instruction:
						 *	In the case above, many threads share a vector(cvgs) and write it occationally.
						 *	SSE instruction "_mm_stream_si32" can write data bypassing cache.
						 *	We use SSE instruction to accelerate parallel writing the vector(cvgs). 
						 *
						 *	Why?
						 *	    Normal cache strategy is "write back". When a thread(cpu) write cvgs, it will write
						 *	the mirror data in it's cache. And this data is synchronized among cpus according
						 *	to MESI protocol. This complex synchronize protocol will occupy much cpu time!
						 *	    SSE instruction can write data bypassing cache. Which can avoid much complex
						 *	synchronization. Cpu write directly to memory and let others cpus' cache line invalid.
						 *	Cpu read the coresponding data line from cache or not accodring to whether it is 
						 *	invalid. Which can avoid the complex synchronization. 
						 *	    Using "volatile" specifier is also a good idea. But it slower than SSE instruction.
						 *	Although volatile variable write directly to memory, but it also read data directly.
						 *	SSE instrucion only writes data directly, but read data through cache. 
						 *		Thus, this SSE instruction is feasible to the scenario where many cpus share some
						 *	common data and write it occationally.
						 *
						 *  Test result: 
						 *    	amzaon-2008.txt not use: 1000ms use: 300ms
						 *    	soc-livejournal1.txt not use: 8826ms use: 3939ms
						 */
					}

					swap_value(value1, value2);

					cvg = 0;
					for(int j = 0; j < tnum; j++)
					{
						cvg += cvgs[j];
						_mm_stream_si32(&(cvgs[j]), 1);
					}
					itr_cnt++;
				}while(cvg != tnum);
			//	cout << "single scc size: " << size << " cvg at: " << itr_cnt << endl;
			}
			else //many small scc
			{
				//cout << "sccs level: size " << ru.second - ru.first << endl;
				#pragma omp parallel for schedule(dynamic, 1000) //1000 is experience
				for(ID i = ru.first; i < ru.second; i++)
				{
					values_t value_a = value1, value_b = value2;
					SccGroupUnit &scc_group = (SccGroupUnit&)fscc_group[i];
					cnt_t size = scc_group.to_id - scc_group.from_id + 1, cvg_cnt = 0;
					do
					{
						cvg_cnt = 0;
						for(ID j = scc_group.from_id; j <= scc_group.to_id; j++)
						{
							addr_t in_base = fvtxs_in.get_vtx_base_addr(j),
							     out_base = fvtxs_out.get_vtx_base_addr(j);
							
						//	if(in_base == NULL && out_base == NULL)
						//		cout << "all null " << map_otn[dfs_map_nto[j]] << endl;
							cvg_cnt += update(
								VtxUnitIdData::id(out_base == NULL ? in_base : out_base),
								VtxUnitIdData::bor(in_base),
								VtxUnitIdData::bor(out_base),
								VtxUnitIdData::bor_cnt(in_base),
								VtxUnitIdData::bor_cnt(out_base),
								value_a, value_b);
						}
						swap_value(value_a, value_b);
						
					//	cout << "size: " << size << " cvg_cnt: " << cvg_cnt << endl;
					}while(size != cvg_cnt);
				}
			}
			//get_time(); cout << endl; 
		}
	}

	void undirect_serial_run()
	{
		cnt_t cvg_cnt = 0, iter_times = 0;
		addr_t in_base, out_base;
		do
		{
			cvg_cnt = 0;
			//forward
			for(ID i = 0; i < vtxs_size; i++)
			{
				in_base = fvtxs_in.get_vtx_base_addr(i);
				out_base = fvtxs_out.get_vtx_base_addr(i);

				cvg_cnt += update(
					VtxUnitIdData::id(out_base == NULL ? in_base : out_base),
					VtxUnitIdData::bor(in_base),
					VtxUnitIdData::bor(out_base),
					VtxUnitIdData::bor_cnt(in_base),
					VtxUnitIdData::bor_cnt(out_base),
					value1, value2);
			}
			swap_value(value1, value2);
			iter_times++;
			
			cout << cvg_cnt << ":" << vtxs_size << endl;
			if(cvg_cnt >= vtxs_size)
				break;
	
			/*
			 *		Backward is inefficient.
			 *		Caes 1: DAG: 1 4 5, 2 5 6, 3 6. 
			 *		Case 2: One direction PathGraph in Forward order. Visiting order is out-of-order when backwarding.
			 *		Fixing problem: Use two direction PathGraph order. But it will increase the amount of
			 *			data read from disk by more than 2 times.
			 */
			//backward 
	/*		cvg_cnt = 0;
			for(ID i = vtxs_size - 1; i != ID_NULL; i--)
			{
				in_base = fvtxs_in.get_vtx_base_addr(i);
				out_base = fvtxs_out.get_vtx_base_addr(i);
			
				cvg_cnt += update(
					VtxUnitIdData::id(out_base == NULL ? in_base : out_base),
					VtxUnitIdData::bor(in_base),
					VtxUnitIdData::bor(out_base),
					VtxUnitIdData::bor_cnt(in_base),
					VtxUnitIdData::bor_cnt(out_base),
					value1, value2);
			}
			swap_value(value1, value2);
			iter_times++;
			cout << cvg_cnt << ":" << vtxs_size << endl;
	*/	
		}while(cvg_cnt < vtxs_size);
		cout << "itera times: " << iter_times << endl;
	}

	void undirect_para_run()
	{
		bool cvg = true;
		int iter_times = 0, tnum = omp_get_max_threads();
		do
		{
			//forward
			cvg = true;
			for(cnt_t i = 0; i < fscc_topo_level.size(); i++)
			{
				RawUnit<ID> &ru = fscc_topo_level[i];
				if(ru.first == ru.second - 1) //big scc
				{
					SccGroupUnit &scc_group = (SccGroupUnit&)fscc_group[ru.first];

					addr_t in_base, out_base;
					vector<int> cvgs(tnum, 1);
#pragma omp parallel for schedule(static, 500) private(in_base, out_base)
					for(ID j = scc_group.from_id; j <= scc_group.to_id; j++)
					{
                         in_base = fvtxs_in.get_vtx_base_addr(j);
                         out_base = fvtxs_out.get_vtx_base_addr(j);

                         if(update(
                                     VtxUnitIdData::id(out_base == NULL? in_base : out_base),
                                     VtxUnitIdData::bor(in_base),
                                     VtxUnitIdData::bor(out_base),
                                     VtxUnitIdData::bor_cnt(in_base),
                                     VtxUnitIdData::bor_cnt(out_base),
                                     value1, value2) == false)
                             _mm_stream_si32(&(cvgs[omp_get_thread_num()]), 0);
					}
					for(int j = 0; j < tnum; j++)
						cvg = cvg & cvgs[j];

				}
				else //small scc
				{
					vector<int> cvgs(tnum, 1);
#pragma omp parallel for schedule(dynamic, 1000)
					for(ID i = ru.first; i < ru.second; i++)
					{
						SccGroupUnit &scc_group = (SccGroupUnit&)fscc_group[i];
						for(ID j = scc_group.from_id; j <= scc_group.to_id; j++)
						{
							
                         addr_t in_base = fvtxs_in.get_vtx_base_addr(j),
								out_base = fvtxs_out.get_vtx_base_addr(j);

                         if(update(
                                     VtxUnitIdData::id(out_base == NULL? in_base : out_base),
                                     VtxUnitIdData::bor(in_base),
                                     VtxUnitIdData::bor(out_base),
                                     VtxUnitIdData::bor_cnt(in_base),
                                     VtxUnitIdData::bor_cnt(out_base),
                                     value1, value2) == false)
                             _mm_stream_si32(&(cvgs[omp_get_thread_num()]), 0);
						}
					}
					for(int j = 0; j < tnum; j++)
						cvg = cvg & cvgs[j];
				}
			}
			swap_value(value1, value2);
			iter_times++;

			//Backward is inefficient. Consult with undirect_serial_run()
		}while(!cvg);
		cout << "itera times: " << iter_times << endl;
	}

    void print_top(int num)
    {
            int gap = vtxs_size / num;
            cout << "Top value:" << endl;
            for (int i = 0; i < num; i++)
				cout << i * gap << ":\t" << value2[map_otn[i * gap]] << endl;
			cout << endl;
    }

	void reset(bool set_is_syn_not_asyn = true)
	{
		if(set_is_syn_not_asyn)
		{
			value1 = value1_bak;
			value2 = value2_bak;
		}
		else
			value1 = value2 = value1_bak;

		for(ID i = 0; i < vtxs_size; i++)
		{
			ID id =  map_nto[i];
			init(id, value1[i]);
			init(id, value2[i]);
		}
	}

private:
	template <class MAXT>
	MAXT max(MAXT a, MAXT b){return a < b ? b : a;}
};

#endif
