#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include "api.h"

using namespace std;

class QWcc : public UnSccDagApp<ID>
{
private:
	vector<bool> is_visit;
	vector<ID> set_cnt;
	ID min(ID a, ID b){return a < b ? a : b;}
public:
	QWcc(string dir, string value_dir): UnSccDagApp(dir, string("qwcc_value.bin"), value_dir)
	{
		is_visit.resize(vtxs_size, false);
		set_cnt.resize(vtxs_size, 1);
	}

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


	void update_mark3(values_t values, ID i, ID &mark)
	{
		ID p;
//#pragma omp critical
		{
		p = find(values, i);
		
		if(mark > p)
		{
			values[mark] = p;
			mark = p;
		}
		else if(mark < p)
			values[p] = mark;
		}
	}

	void update_mark2(values_t values, ID i, ID &mark)
	{
		if(i < mark)
		{
			ID p = mark;
			while(i < values[p])
				p = values[p];
			//if(i != p)
			values[i] = values[p];
			values[p] = i;
		}
		else if(i > mark)
		{
			ID p = i;
			while(mark < values[p])
				p = values[p];
			//if(i != mark)
			values[mark] = values[p];
			values[p] = mark;
		}
		mark = find(values, mark);
		mark = find(values, i);
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
			
			addr_t out_base = fvtxs_out.get_vtx_base_addr(i);
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
		/*
			addr_t in_base = fvtxs_in.get_vtx_base_addr(i);
			size = VtxUnitIdData::bor_cnt(in_base);
			ID *inbor = VtxUnitIdData::bor(in_base);
			for(int j = 0; j < size; j++)
			{
				ID inid = inbor[j];
				if(!is_visit[inid])
					ids.push(inid);
				else
				{
					if(mark > values[inid])
					{
						values[mark] = values[inid];
						mark = values[inid];
					}
				}
			}	
		*/
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
			
			addr_t out_base = fvtxs_out.get_vtx_base_addr(i);
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
		values_t values = value1;
		//mark vtxs
		cout << "mark vtxs" << endl;
		#pragma omp parallel for schedule(static)
		for(ID i = 0; i < vtxs_size; i++)
		{
			/*
			 *	Why use BFS:  
			 *	We use bfs instead of dfs to traverse graph , because dfs will encounter much data racing.
			 *	For speed, we have ignored protecting the data of para disjoint-set.(value1)
			 *	Data racing will leads to uncertainty when there is nothing todo with protection.
			 *	Experements show that bfs is more stable than dfs.(amazon, enwiki, soc-poket, twitter)
			 */
			if(!is_visit[i])
				bfs(values, i);
		}

		//merge mark
		cout << "merge mark" << endl;
		#pragma omp parallel for schedule(static)
		for(ID i = 0; i < vtxs_size; i++)
		{
			find(values, i);
		}
		value2 = value1;
	}

	void print_wcc()
	{
		cnt_t cnt = 0;
		cout << "WCCs:" << endl;
		for(ID i = 0; i < vtxs_size; i++)
		{
			if(i == value1[i])
			{
				//cout << "wcc-" << i << endl;
				cnt++;
			}
		}
		cout << "wcc num: " << cnt << endl;
		cout << endl;
	}

	void init(ID id, value_t &value){value = id;};
	bool update(ID virtual_id, ID* inbor, ID* outbor, cnt_t in_cnt, cnt_t out_cnt,
          const values_t rvalue, values_t wvalue){ return true;};
	
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



