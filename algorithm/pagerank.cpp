#include "api.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>

using namespace std;

template <class App>
class PageRankApp : public App
{
private:
	const typename App::value_t P = 0.85;
	const typename App::value_t DELTA = 1 - P;
	const typename App::value_t SVAL = 1;
	const typename App::value_t EPS = SVAL / 10000; // 0.0001; 
public:
	PageRankApp(string dir, string value_dir): App(dir, string("pagerank_value.bin"), value_dir){};

	void init(ID id, typename App::value_t &value)
	{
		value = SVAL;
	}

	bool update(ID virtual_id, typename App::bor_t inbor, typename App::bor_t outbor, cnt_t in_cnt, cnt_t out_cnt,
		 const typename App::values_t rvalue, typename App::values_t wvalue)
	{
		//cout << "calc: " << App::map_nto[virtual_id] << endl;
		typename App::value_t val = 0;
		//*
        for (int i = 0; i < in_cnt; i++)
        {
			val += rvalue[inbor[i]];
		}
		//*/
		/*
		ID idval = 0;
		PG_Foreach(inbor, idval)
		{
			val += rvalue[idval];
		}
		//*/
        val = val * P + DELTA;

        if (out_cnt != 0)
           	val /= (typename App::value_t)(out_cnt);

        bool is_cvg = (fabs(val - wvalue[virtual_id]) < EPS); 
		//Bug: must use wvalue to evaluate is_cvg, because many cvg at EPS will accumulate and leads to non cvg.
        	
		wvalue[virtual_id] = val;
		
        //return fabs(val - rvalue[virtual_id]) < EPS;
        return is_cvg;
	}
};

int main(int argc, char *argv[])
{
        if (argc != 3 && argc != 4)
        {
                cout << "Usage ./pagerank <data dir> <generate value to dir> <print top(default 10)>" << endl;
                return 0;
        }

	int top_num = (argc == 4 ? top_num = atoi(argv[3]) : 10) ;

	string dir(argv[1]), to_dir(argv[2]);
///*
	cout << "normal para run" << endl; get_time(false);
	PageRankApp<UnSccDagApp<float>> upr(dir, to_dir);	
	upr.reset(true);
	upr.para_run();
	get_time(); cout << endl;
	upr.print_top(top_num);
//*/
	cout << "para run" << endl; get_time(false);
	PageRankApp<SccDagApp<float>> pr(dir, to_dir);
	pr.reset(true);
	pr.para_run();
	get_time(); cout << endl;
	pr.print_top(top_num);

	return 0;
}
