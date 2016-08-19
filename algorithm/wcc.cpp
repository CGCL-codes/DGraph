#include "api.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>

using namespace std;

template <class App>
class WCCApp : public App
{
public:
	WCCApp(string dir, string value_dir): App(dir, string("cc_value.bin"), value_dir){};

	int min(int a, int b){return a < b ? a : b;}	

	void init(ID id, typename App::value_t &value)
	{
		value = (int)(id);
	}

	bool update(ID virtual_id, typename App::bor_t inbor, typename App::bor_t outbor, cnt_t in_cnt, cnt_t out_cnt,
		 const typename App::values_t rvalue, typename App::values_t wvalue)
	{
		int val = rvalue[virtual_id];

		for (int i = 0; i < in_cnt; i++)
           	val = min(val, rvalue[inbor[i]]);
		for (int i = 0; i < out_cnt; i++)
           	val = min(val, rvalue[outbor[i]]);

        bool is_cvg = (wvalue[virtual_id] == val);

		//if(!is_cvg)
		//	cout << App::map_nto[virtual_id] << ": " << val << "->" << wvalue[virtual_id] << endl;

        wvalue[virtual_id] = val;

        return is_cvg;
	}

    void print_wcc()
    {
		cnt_t cnt = 0;
        cout << "WCCs:" << endl;
        for(ID i = 0; i < App::vtxs_size; i++)
        {
            if(App::map_nto[i] == App::value1[i])
			{
				//cout << "wcc-" << App::map_nto[i] << endl;
				cnt++;
			}
        }
		cout << "wcc num: " << cnt << endl;
        cout << endl;
     }

};

int main(int argc, char *argv[])
{
        if (argc != 3 && argc != 4)
        {
                cout << "Usage ./wcc <data dir> <generate value to dir> <print top(default 10)>" << endl;
                return 0;
        }

	int top_num = (argc == 4 ? top_num = atoi(argv[3]) : 10) ;

	string dir(argv[1]), to_dir(argv[2]);

	cout << "normal para run" << endl; get_time(false);
	WCCApp<UnSccDagApp<int>> ucc(dir, to_dir);
	ucc.reset(false);
	ucc.para_run();
	get_time(); cout << endl;
	ucc.print_top(top_num);
	ucc.print_wcc();

	cout << "undirect para run" << endl; get_time(false);
	WCCApp<SccDagApp<int>> cc(dir, to_dir);
	cc.reset(false);
	cc.undirect_para_run();
	get_time(); cout << endl;
	cc.print_top(top_num);
	cc.print_wcc();
	return 0;
}
