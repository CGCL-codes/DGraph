#include "api.h"
#include "debug.h"
#include <iostream>
#include <string>

using namespace std;

class Demo : public SccDagApp<long long>
{
public:
	Demo(string dir, string value_dir): SccDagApp(dir, string("demo_value.bin"), value_dir){};

	void init(ID id, value_t &value)
	{
		value = (long long)id;
	}

	bool update(ID virtual_id, typename SccDagApp::bor_t inbor, typename SccDagApp::bor_t outbor, 
			bor_cnt_t in_cnt, bor_cnt_t out_cnt, const values_t rvalue, values_t wvalue)
	{
		long long x = 0;
		for(ID i = 0; i < in_cnt; i++)
			x += rvalue[inbor[i]];
		x = out_cnt == 0 ? 0 : (x + out_cnt) % out_cnt;
	
		wvalue[virtual_id] = x;
	
		return (x == rvalue[virtual_id]); 
	}
};

int main(int argc, char *argv[])
{
        if (argc != 3)
        {
                cout << "Usage ./rundemo <data dir> <generate value to dir>" << endl;
                return 0;
        }

	string dir(argv[1]), to_dir(argv[2]);
///*
	cout << "serial run"; get_time(false);
	Demo demo1(dir, to_dir);
	demo1.reset();
	demo1.normal_para_run(); 
	get_time(); cout << endl;
	demo1.print_top(10);
//*/
	cout << "para run"; get_time(false);
	Demo demo2(dir, to_dir);
	demo2.reset();
	demo2.para_run();
	get_time(); cout << endl;
	demo2.print_top(10);

	return 0;
}
