/*
 * Community Detection algorithm: LPA (Label Propagation Algorithm)
 * This algorithm had better be asynchronous, synchronous may not to converge.
 */
#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <map>
#include "api.h"

using namespace std;

template <class App>
class LPAApp : public App
{
public:
	LPAApp(string dir, string value_dir): App(dir, string("cc_value.bin"), value_dir){};

	int min(int a, int b){return a < b ? a : b;}	

	void init(ID id, typename App::value_t &value)
	{
		value = (int)(id);
	}

	bool update(ID virtual_id, typename App::bor_t inbor, typename App::bor_t outbor, cnt_t in_cnt, cnt_t out_cnt,
		 const typename App::values_t rvalue, typename App::values_t wvalue)
	{
		int max_cnt = 0;
		ID max_id = rvalue[virtual_id];
		map<ID, int> lcnt;

		for(int i = 0; i < in_cnt; i++)
		{
			ID id = rvalue[inbor[i]];
			lcnt[id] = lcnt[id] + 1;
			if(max_cnt < lcnt[id] || (max_cnt == lcnt[id] && id > max_id))
			{
				max_cnt = lcnt[id];
				max_id = id;
			}
		}

		//cout << App::map_nto[virtual_id] << ": "<< wvalue[virtual_id] << "-" << max_id << endl;
		bool is_cvg = (wvalue[virtual_id] == max_id);
		wvalue[virtual_id] = max_id;

       	return is_cvg;
	}
};

int main(int argc, char *argv[])
{
        if (argc != 3 && argc != 4)
        {
                cout << "Usage ./lpa <data dir> <generate value to dir> <print top(default 10)>" << endl;
                return 0;
        }

	int top_num = (argc == 4 ? top_num = atoi(argv[3]) : 10) ;

	string dir(argv[1]), to_dir(argv[2]);

	cout << "normal para run" << endl; get_time(false);
	LPAApp<UnSccDagApp<ID>> ulpa(dir, to_dir);
	ulpa.reset(false);
	ulpa.para_run();
	get_time(); cout << endl;
	ulpa.print_top(top_num);

	cout << "para run" << endl; get_time(false);
	LPAApp<SccDagApp<ID>> lpa(dir, to_dir);
	lpa.reset(false);
	lpa.para_run();
	get_time(); cout << endl;
	lpa.print_top(top_num);

	return 0;
}
