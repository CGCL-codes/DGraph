#ifndef SCC_DAG_TYPE
#define SCC_DAG_TYPE

typedef unsigned int ID;
typedef unsigned long long offset_t;
typedef long long cnt_t;
typedef int bor_cnt_t;
typedef unsigned char* addr_t;

const ID ID_NULL = (ID)0 - 1;

enum RType{
	PageRank = 1,
	CC = 2,
	SSSP = 3
};

enum Status{
	OK = 1,
	ERROR = 0
};

#endif

