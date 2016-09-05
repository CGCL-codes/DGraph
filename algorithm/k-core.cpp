#include "api.h"
#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>

using namespace std;

template <class App>
class KCore : public App
{
private:
	int K;
public:
	KCore(string dir, string value_dir, int K): App(dir, string("k_core_value.bin"), value_dir), K(K){};

	void init(ID id, typename App::value_t &value)
	{
		value = true;
	}

	bool update(ID virtual_id, typename App::bor_t inbor, typename App::bor_t outbor,
			bor_cnt_t in_cnt, bor_cnt_t out_cnt, const typename App::values_t rvalue, typename App::values_t wvalue)
	{
		int cnt = 0;
		/*	
		for(int i = 0; i < in_cnt; i++)
				cnt += rvalue[inbor[i]];
		//*/
		//*
		ID idval = 0;
		PG_Foreach(inbor, idval)
			cnt += rvalue[idval];
		//*/
		bool is_cvg = !(wvalue[virtual_id] ^ (cnt >= K));
		wvalue[virtual_id] = (cnt >= K);
		return is_cvg;
	}

	void print_top(int top_num)
	{
		cnt_t core_vtx_num = 0;
		for(int i = 0; i < App::vtxs_size; i++)
			core_vtx_num += App::value2[i];
		cout << "K-Core vtxs num: " << core_vtx_num << endl;
		App::print_top(top_num);
	}

};

int main(int argc, char *argv[])
{
        if (argc != 3 && argc != 4 && argc != 5)
        {
                cout << "Usage ./k-core <data dir> <generate value to dir> <K(default 5)> <kc.nt top(default 10)>" << endl;
                return 0;
        }

	int top_num = (argc == 5 ? top_num = atoi(argv[4]) : 10),
		K = (argc >= 4 ? atoi(argv[3]) : 5);

	string dir(argv[1]), to_dir(argv[2]);
/*
	cout << "normal para run" << endl; get_time(false);
	KCore<UnSccDagApp<bool>> ukc(dir, to_dir, K);	
	ukc.reset(false);
	ukc.para_run();
	get_time(); cout << endl;
	ukc.kc.nt_top(top_num);
//*/
	cout << "para run" << endl; get_time(false);
	KCore<SccDagApp<bool>> kc(dir, to_dir, K);
#if defined(SD_NORMAL_SYNC)
	cout << "normal sync para run" << endl; get_time(false);
	kc.reset();
	kc.normal_para_run();
#elif defined(SD_SYNC)
	cout << "sync para run" << endl; get_time(false);
	kc.reset();
	kc.para_run();
#elif defined(SD_NORMAL_ASYNC)
	cout << "normal asyn para run" << endl; get_time(false);
	kc.reset(false);
	kc.normal_para_run();
#else
	cout << "asyn para run" << endl; get_time(false);
	kc.reset(false);
	kc.para_run();
#endif
	get_time(); cout << endl;
	kc.print_top(top_num);

	return 0;
}
