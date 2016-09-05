#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include "api.h"

using namespace std;

class QWcc : public SccDagApp<ID>
{
private:
	vector<bool> is_visit;
	vector<ID> set_cnt;
	typename SccDagApp::values_t sets;
	ID min(ID a, ID b){return a < b ? a : b;}
public:
	QWcc(string dir, string value_dir): SccDagApp(dir, string("qwcc_value.bin"), value_dir)
	{
		is_visit.resize(scc_size, false);
		set_cnt.resize(scc_size, 1);
		sets = (values_t)malloc(sizeof(value_t) * scc_size); //before calc, need reset
	}

	~QWcc(){free(sets);}

	ID find(values_t values, ID id)
	{
		while(values[id] != id)
			id = values[id] = values[values[id]];
		return values[id];
	}

	void union_update_mark(values_t values, ID i, ID &mark)
	{
		mark = find(values, mark);
		i = find(values, i);
	
		if(i == mark)
			return;

		if(set_cnt[i] > set_cnt[mark])
		{
			values[mark] = i;
			set_cnt[i] += set_cnt[mark];
			mark = i;
		}
		else
		{
			values[i] = mark;
			//_mm_stream_si32((int*)&(set_cnt[mark]), int( set_cnt[mark] + set_cnt[i]));
			set_cnt[mark] += set_cnt[i];
		}
	}

	void dfs(values_t values, ID id)
	{
		ID mark = id;
		stack<ID> ids;
		ids.push(id);
		while(!ids.empty())
		{
			ID i = ids.top();
			ids.pop();

			//union_update_mark(values, i, mark);
			if(is_visit[i])
			{
				union_update_mark(values, i, mark);
				continue;
			}
			is_visit[i] = true;

			if(i < mark)
			{
				values[mark] = i;
				set_cnt[i] += set_cnt[mark];
				mark = i;
			}
			else
			{
				values[i] = mark;
				set_cnt[mark]++;
			}
			
			addr_t out_base = fscc_vtx.get_vtx_base_addr(i);
			cnt_t size = VtxUnitIdData::bor_cnt(out_base);
			ID *outbor = VtxUnitIdData::bor(out_base);
			for(int j = 0; j < size; j++)
			{
				ID outid = outbor[j];
				if(!is_visit[outid])
					ids.push(outid);
				else
					union_update_mark(values, outid, mark);
			}
		}
	}

	void bfs(values_t values, ID id)
	{
		ID mark = id;
		queue<ID> ids;
		ids.push(id);
		while(!ids.empty())
		{
			
			ID i = ids.front();
			ids.pop();
			
			union_update_mark(values, i, mark);
			if(is_visit[i])
				continue;

			is_visit[i] = true;
			
			addr_t out_base = fscc_vtx.get_vtx_base_addr(i);
			cnt_t size = VtxUnitIdData::bor_cnt(out_base);
			ID *outbor = VtxUnitIdData::bor(out_base);
			for(int j = 0; j < size; j++)
			{
				ID outid = outbor[j];
				if(!is_visit[outid])
					ids.push(outid);
				else
					union_update_mark(values, outid, mark);
			}
		}
	}

	void calc()
	{
		//Mark scc vtxs
		cout << "mark scc vtxs" << endl;
		#pragma omp parallel for schedule(static)
		for(ID i = 0; i < scc_size; i++)
		{
			/*
			 *	Why use BFS:  
			 *	We use bfs instead of dfs to traverse graph , because dfs will encounter much data racing.
			 *	For speed, we have ignored protecting the data of para disjoint-set.(value1)
			 *	Data racing will leads to uncertainty when there is nothing todo with protection.
			 *	Experements show that bfs is more stable than dfs.(amazon, enwiki, soc-poket, twitter)
			 */
			if(!is_visit[i])
				bfs(sets, i);
		}

		//Merge mark
		cout << "merge mark" << endl;
		#pragma omp parallel for schedule(static)
		for(ID i = 0; i < scc_size; i++)
		{
			find(sets, i);
		}

		//Scatter scc_vtx's wcc to normal vtxs
		cout << "mark vtxs" << endl;

		ID *wcc_to_vtx = new ID[scc_size];

		#pragma omp parallel for
		for(ID i = 0; i < scc_size; i++)
			wcc_to_vtx[i] = ID_NULL;

		#pragma omp parallel for //schedule(dynamic, 100)
		for(ID i = 0; i < scc_size; i++) //get wcc_to_vtx
		{
			ID wcc_i = find(sets, i); 
			if(wcc_to_vtx[wcc_i] == ID_NULL)
			{
				SccGroupUnit &scc_group = (SccGroupUnit&)fscc_group[wcc_i];
				addr_t out_base = fvtxs_out.get_vtx_base_addr(scc_group.from_id);
			//	cout << wcc_i << "-" << VtxUnitIdData::id(out_base) << endl;
				wcc_to_vtx[wcc_i] = VtxUnitIdData::id(out_base);
			}
		}

		#pragma omp parallel for schedule(dynamic, 100)
		for(ID i = 0; i < scc_size; i++) // mark vtxs
		{
			ID wcc_i = find(sets, i);
			SccGroupUnit &scc_group = (SccGroupUnit&)fscc_group[i];
			#pragma omp parallel for schedule(dynamic, 1024)
			for(ID j = scc_group.from_id; j <= scc_group.to_id; j++)
			{
				addr_t out_base = fvtxs_out.get_vtx_base_addr(j);
				ID vtx_i = VtxUnitIdData::id(out_base);
				//cout << vtx_i << "-" << wcc_to_vtx[wcc_i] << endl;
				value2[vtx_i] = wcc_to_vtx[wcc_i];
			}
		}
	}

	void print_wcc()
	{
		cnt_t cnt = 0;
		cout << "WCCs:" << endl;
		for(ID i = 0; i < scc_size; i++)
		{
			if(i == sets[i])
			{
				//cout << "wcc-" << i << endl;
				cnt++;
			}
		}
		cout << "wcc num: " << cnt << endl;
		cout << endl;
	}

	void init(ID id, value_t &value){value = id;};
	bool update(ID virtual_id, typename SccDagApp::bor_t inbor, typename SccDagApp::bor_t outbor, 
			bor_cnt_t in_cnt, bor_cnt_t out_cnt,
          const values_t rvalue, values_t wvalue){ return true;};
	void reset(bool set_is_syn_not_asyn = true)
	{
		for(ID i = 0; i < scc_size; i++) sets[i] = i;
		SccDagApp::reset(set_is_syn_not_asyn);
	}
	
};

int main(int argc, char *argv[])
{
	if(argc != 3 && argc != 4)
    {
	    cout << "Usage ./qwcc <data dir> <generate value to dir> <print top(default 10)>" << endl;
	    return 0;
	}
    int top_num = (argc == 4 ? top_num = atoi(argv[3]) : 10) ;

	string dir(argv[1]), to_dir(argv[2]);

	cout << "normal para run" << endl; get_time(false);
	QWcc qwcc(dir, to_dir);

	qwcc.reset(false);
	qwcc.calc();
	get_time(); cout << endl;
	qwcc.print_top(top_num);
	qwcc.print_wcc();	
	return 0;
}



