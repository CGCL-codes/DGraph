#ifndef SCC_DAG_DEBUG_H
#define SCC_DAG_DEBUG_H


//#define DBG_UPDATE_NUM


#include <iostream>
#include <assert.h>
#include <sys/time.h>
#include <string>

using namespace std;

long long get_current_time()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

time_t clock_time;
time_t get_time(bool isshow = true)
{
	time_t now = get_current_time();
	if (isshow)
		std::cout << "(Time: " << now << " delta: " << now - clock_time << ")";
	clock_time = now;
	return now;
}


class DBG_print
{
public:
	DBG_print(string s)
	{
		cout << "DBG: " << s << endl;
	}
};

#endif

