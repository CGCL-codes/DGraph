#include "api.h"
#include "debug.h"
#include <iostream>
#include <string>
#include <cmath>

using namespace std;

class PageRankApp : public SccDagApp<double>
{
public:
	const double P = 0.85;
	const double DELTA = 1 - P;
	const double START_VAL = 1;
	const double EPS = 0.0001;

	PageRankApp(string dir, string value_dir): SccDagApp(dir, string("pr_value.bin"), value_dir){};

	void init(ID id, value_t &value)
	{
		value = START_VAL;
	}

	bool update(ID virtual_id, typename SccDagApp::bor_t inbor, typename SccDagApp::bor_t outbor, cnt_t in_cnt, cnt_t out_cnt,
		const values_t rvalue, values_t wvalue)
	{
		double val = 0;
		for(int i = 0; i < in_cnt; i++)
			val += rvalue[inbor[i]];

		val = val * P + DELTA;

		if(out_cnt != 0)
			val /= (double)out_cnt;

		bool is_cvg = fabs(val - wvalue[virtual_id]) < EPS;

		wvalue[virtual_id] = val;

		return is_cvg;
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
	PageRankApp demo1(dir, to_dir);
	demo1.reset();
	demo1.serial_run(); 
	get_time(); cout << endl;
	demo1.print_top(10);
//*/
	cout << "para run"; get_time(false);
	demo1.reset();
	demo1.para_run();
	get_time(); cout << endl;
	demo1.print_top(10);

	return 0;
}
