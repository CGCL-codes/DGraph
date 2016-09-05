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

typedef struct SccGroupUnit
{
	ID from_id, to_id;
}SccGroupUnit;

template <class T>
class SccDagApp
{
protected:
	//PGVtxsFile<ID> fvtxs_out, fvtxs_in;
	PGVtxsFile<ID> fvtxs_out, fvtxs_in;
	RawFile<ID> fscc_group, fscc_topo_level;
	VtxIndexFile<ID> map_nto, map_otn, vid_order;//, dfs_map_nto, dfs_map_otn;

	cnt_t vtxs_size, scc_size;

#ifdef DBG_UPDATE_NUM
	//For test
	cnt_t update_times; //Only calc in para_run()
#endif

	//typedef VtxIndexFile<T> values_t;
	typedef T value_t;
	typedef T* values_t;
	typedef addr_t bor_t;
	//typedef PGVtxBor_t<ID> bor_t;
	values_t value1, value2, value1_bak, value2_bak;

	void swap_value(values_t &a, values_t &b){values_t t = a; a = b; b = t;}

public:
	SccDagApp(string dir, string value_name, string value_dir, int tnum = omp_get_max_threads()): 
		fvtxs_out("pg_vtxs_out.bin", "index_pg_vtxs_out.bin", dir),
		fvtxs_in("pg_vtxs_in.bin", "index_pg_vtxs_in.bin", dir),
		vid_order("calc_vid_order.bin", dir),
		fscc_group("scc_group.bin", dir),
		fscc_topo_level("scc_topo_level.bin", dir),
		map_nto("pg_vid_to_id.bin", dir),// PathGraph map
		map_otn("pg_id_to_vid.bin", dir)//,// PathGraph map

		//dfs_map_nto("vtxs_map_new_to_old.bin", dir), //no PathGraph map
		//dfs_map_otn("vtxs_map_old_to_new.bin", dir)  //no PathGraph map
		//value(value_name, value_dir)
		{
			cout << "Loading SCC DAG data..." << endl;
			fvtxs_out.open();
			fvtxs_in.open();
			vid_order.open();
			fscc_group.open();
			fscc_topo_level.open();
			map_nto.open();
			map_otn.open();
			//dfs_map_nto.open();
			//dfs_map_otn.open();
		
			//Initialize vtxs
			cout << "initialize vtxs data ";
			vtxs_size = max(fvtxs_in.size(), fvtxs_out.size());
			cout << "(vtxs num: " << vtxs_size << ")" << endl;

			//Initialize value;
			cout << "Initialize value container" << endl;
			//value.open();
			value1 = value1_bak = (T*)malloc(vtxs_size * sizeof(T));
			value2 = value2_bak = (T*)malloc(vtxs_size * sizeof(T));

			//Initialize scc_vtxs
			cout << "Initialize scc data" << endl;
			scc_size = fscc_group.size();
			cout << "Load success." << endl;

			if(tnum > 0) omp_set_num_threads(tnum); 
			cout << "Thread num: " << omp_get_max_threads() << endl;
		};

	~SccDagApp()
	{
		fvtxs_out.close();
		fvtxs_in.close();
		vid_order.close();
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
	virtual bool update(ID virtual_id, bor_t inbor, bor_t outbor, bor_cnt_t in_cnt, bor_cnt_t out_cnt,
				 const values_t rvalue, values_t wvalue) = 0; 

	void para_run()
	{
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
#pragma omp parallel for schedule(dynamic, 1000) 
//#pragma omp parallel for schedule(static) private(in_base, out_base)
					for(ID j = scc_group.from_id; j <= scc_group.to_id; j++)
					{
						//out_base will never be null, we construct it for full vertix data

						if(update( 
									vid_order[j],
									fvtxs_in[j],
									fvtxs_out[j], 
									fvtxs_in.degree(j),
									fvtxs_out.degree(j),
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

#ifdef DBG_UPDATE_NUM
					//For test
					update_times += scc_group.to_id - scc_group.from_id + 1;
#endif

				}while(cvg != tnum);
				//cout << "single scc size: " << size << " cvg at: " << itr_cnt << endl;
			}
			else //many small scc
			{
#ifdef DBG_UPDATE_NUM
				//For test
				int tnum = omp_get_max_threads();
				vector<cnt_t> t_cnt(tnum, 0);
				//volatile cnt_t *t_cnt = new cnt_t [tnum]();
				//for(int i = 0; i < tnum; i++)
					//t_cnt[i] = 0;
#endif

			//	cout << "sccs level: size " << ru.second - ru.first << endl;
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
								cvg_cnt += update(
									vid_order[j],
									fvtxs_in[j],
									fvtxs_out[j], 
									fvtxs_in.degree(j),
									fvtxs_out.degree(j),
									value_a, value_b);
						}
						swap_value(value_a, value_b);

#ifdef DBG_UPDATE_NUM
						t_cnt[omp_get_thread_num()] += size;
						//cnt_t cnt = t_cnt[omp_get_thread_num()]; 
						//_mm_stream_si64(&(t_cnt[omp_get_thread_num()]),
								//cnt + size);  //SSE instruction 
#endif	
					}while(size != cvg_cnt);
				}

#ifdef DBG_UPDATE_NUM
				for(int i = 0; i < tnum; i++)
					update_times += t_cnt[i];
#endif
			}
			//get_time(); cout << endl; 
		}
	}

	void normal_para_run()
	{
		//reset();
		
		int cvg = 0, iter_times = 0;
		int tnum = omp_get_max_threads();
		vector<int> cvgs(tnum, 1);	
		cnt_t size = vtxs_size;
		do
		{
			//Normal run strategy divides the partition by 1000 units and adopt dynamic schedule (not equal distribution).
			#pragma omp parallel for schedule(dynamic, 1000)
			//#pragma omp parallel for schedule(static) private(in_base, out_base)
			for(ID i = 0; i < size; i++)
			{
				//cout  << fvtxs_in.vid(i) << "-" << fvtxs_out.vid(i) << endl;
				if(update(
						vid_order[i],
						fvtxs_in[i],
						fvtxs_out[i], 
						fvtxs_in.degree(i),
						fvtxs_out.degree(i),
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
#ifdef DBG_UPDATE_NUM
			update_times += size;
#endif
		}while(cvg != tnum);
		cout << "itera times: " << iter_times << endl;	
	}

    void print_top(int num)
    {
            int gap = vtxs_size / num;
            cout << "Top value:" << endl;
            for (int i = 0; i < num; i++)
				cout << i * gap << ":\t" << value2[map_otn[i * gap]] << endl;
			cout << endl;
#ifdef DBG_UPDATE_NUM
			cout << "Update times: " << update_times << endl;
#endif
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

#ifdef DBG_UPDATE_NUM
		//For test
		update_times = 0;
#endif
	}

private:
	template <class MAXT>
	MAXT max(MAXT a, MAXT b){return a < b ? b : a;}
};

#endif
