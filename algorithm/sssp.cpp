#include "api.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>
#include <float.h>

using namespace std;

template<class App>
class SSSPApp : public App
{
private:
	const double EPS = 0.0001;
	ID root;
public:
	SSSPApp(ID root, string dir, string value_dir): root(root), App(dir, string("sssp_value.bin"), value_dir){};

	double min(double a, double b){return a < b ? a : b;}	

	//distance is a hash value of from_id and to_id.
	double distance(ID from, ID to){return double((from + to) % 100);}

	void init(ID id, typename App::value_t &value)
	{
		if(id == root)
			value = 0;
		else value = DBL_MAX - 1;
	}

	bool update(ID virtual_id, typename App::bor_t inbor, typename App::bor_t outbor, bor_cnt_t in_cnt, bor_cnt_t out_cnt,
			typename App::values_t rvalue, typename App::values_t wvalue)
	{
		double d = rvalue[virtual_id];
		ID from_id =  App::map_nto[virtual_id];
		/*
		for(int i = 0; i < in_cnt; i++)
		{
			ID to_id = App::map_nto[inbor[i]];
			d = min(d, rvalue[inbor[i]] + distance(from_id, to_id));
		}
		//*/
		//*
		ID idval = 0, to_id = 0;
		PG_Foreach(inbor, idval)
		{
			to_id = App::map_nto[idval];
			d = min(d, rvalue[idval] + distance(from_id, to_id));
		}
		//*/
		bool is_cvg = (fabs(d - wvalue[virtual_id]) < EPS);
		wvalue[virtual_id] = d;
        return is_cvg;
	}
};

int main(int argc, char *argv[])
{
    if (argc != 3 && argc != 4 && argc != 5)
    {
        cout << "Usage ./sssp <data dir> <generate value to dir> <root(default 0)> <print top(default 10)>" << endl;
        return 0;
    }

	ID root = ((argc == 4 || argc == 5) ? atoll(argv[3]) : 0);
	int top_num = (argc == 5 ? atoi(argv[4]) : 10) ;

	string dir(argv[1]), to_dir(argv[2]);
	/*
	cout << "normal para run"; get_time(false);
	SSSPApp<UnSccDagApp<double>> usssp(root, dir, to_dir);
	usssp.reset(false);
	usssp.para_run(); 
	get_time(); cout << endl;
	usssp.print_top(top_num);
	//*/
	SSSPApp<SccDagApp<double>> sssp(root, dir, to_dir);
#if defined(SD_NORMAL_SYNC)
	cout << "normal sync para run" << endl; get_time(false);
	sssp.reset();
	sssp.normal_para_run();
#elif defined(SD_SYNC)
	cout << "sync para run" << endl; get_time(false);
	sssp.reset();
	sssp.para_run();
#elif defined(SD_NORMAL_ASYNC)
	cout << "normal asyn para run" << endl; get_time(false);
	sssp.reset(false);
	sssp.normal_para_run();
#else
	cout << "asyn para run" << endl; get_time(false);
	sssp.reset(false);
	sssp.para_run();
#endif
	get_time(); cout << endl;
	sssp.print_top(top_num);

	return 0;
}
