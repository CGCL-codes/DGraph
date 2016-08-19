#include "api.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <float.h>
#include <limits.h>

using namespace std;

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

	bool update(ID virtual_id, typename App::bor_t inbor, typename App::bor_t outbor, cnt_t in_cnt, cnt_t out_cnt,
		const typename App::values_t rvalue, typename App::values_t wvalue)
	{
		int d = rvalue[virtual_id];

		///*
		//cout << virtual_id << ": ";
		for(int i = 0; i < in_cnt; i++)
		{
		//	cout << inbor[i] << " ";
			d = min(d, rvalue[inbor[i]] + 1);
		}
		//cout << endl;
		//*/
		/*
		ID idval = 0;
		PG_Foreach(inbor, idval)
		{
			d = min(d, rvalue[idval] + 1);
		}
		//*/

		bool is_cvg = (d == wvalue[virtual_id]);
		wvalue[virtual_id] = d;
        	return is_cvg;
	}
};

int main(int argc, char *argv[])
{
    if (argc != 3 && argc != 4 && argc != 5)
    {
            cout << "Usage ./bfs <data dir> <generate value to dir> <root(default 0)> <print top(default 10)>" << endl;
            return 0;
    }

	ID root = ((argc == 4 || argc == 5) ? atoll(argv[3]) : 0);
	int top_num = (argc == 5 ? top_num = atoi(argv[4]) : 10) ;

	string dir(argv[1]), to_dir(argv[2]);
//*
	cout << "normal para run" << endl; get_time(false);
	BFSApp<UnSccDagApp<int>> ubfs(root, dir, to_dir);
	ubfs.reset();
	ubfs.para_run();
	get_time(); cout << endl;
	ubfs.print_top(top_num);
//*/
	cout << "para run" << endl; get_time(false);
	BFSApp<SccDagApp<int>> bfs(root, dir, to_dir);
/*
	bfs.reset();
	cout << "para normal run"; get_time(false);
	bfs.para_normal_run(); 
	get_time(); cout << endl;
	bfs.print_top(top_num);
//*/
/*
	bfs.reset();
	cout << "para scc run"; get_time(false);
	bfs.para_scc_run();
	get_time(); cout << endl;
	bfs.print_top(top_num);
*/
	bfs.reset();
	bfs.para_run();
	get_time(); cout << endl;
	bfs.print_top(top_num);

	return 0;
}
