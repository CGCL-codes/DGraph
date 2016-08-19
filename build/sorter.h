#ifndef SORTER_H
#define SORTER_H

#include "gfile.h"
#include <queue>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <assert.h>
#include <omp.h>

class Sorter
{
public:
	const static unsigned long long DEFAULT_PART_SIZE = 0x40000000 / (sizeof(RawUnit<ID>)) * (2); // 2G (why not 2G + 1 ?)
	static unsigned long long PART_SIZE;

	//Sort Rawfile
	static void sort_raw1(RawFile<ID> &in_raw, RawFile<ID> &out_raw, bool is_by_first, 
			unsigned long long part_size = DEFAULT_PART_SIZE);
	static void sort_raw2(RawFile<ID> &in_raw, RawFile<ID> &out_raw, bool is_by_first,
			int tnum = omp_get_max_threads());

	static void sort_raw(RawFile<ID> &in_raw, RawFile<ID> &out_raw, bool is_by_first,
                         unsigned long long part_size = DEFAULT_PART_SIZE)
	{
		//sort_raw2(in_raw, out_raw, is_by_first, 5);
		//return;
		unsigned long long mem_limit_size = part_size * (unsigned long long)omp_get_max_threads();
		cout << "rawfile size: " << in_raw.size() << " men limit: " << mem_limit_size << endl;
		if(in_raw.size() > mem_limit_size) //out of memory
			sort_raw1(in_raw, out_raw, is_by_first, part_size);
		else	//in memory
			sort_raw2(in_raw, out_raw, is_by_first);
	}
private:
	static long long min(long long a, long long b){return a < b ? a : b;}
};

template<class T>
class UpdateTopHeap
{
private:
	bool (*comp)(T &, T &);
	vector<T> vec;
	unsigned long long hsize, depth; //depth start from 0
	
	void check_resize_(unsigned long long len)
	{
		while(vec.size() - 1 < len)
			vec.resize(vec.size() + (1 << ++depth));
	}

	template <class Tx>
	void swap(Tx &a, Tx &b){Tx t = a; a = b; b = t;}
	//{a ^= b; b ^= a; a ^= b;}
	
public: 
	UpdateTopHeap(bool (*comp)(T &, T &)):comp(comp), hsize(0), depth(3){vec.resize(16);};	

	void percolate_up()
	{
		unsigned long long i = hsize;
		while(i != 1 && !comp(vec[i / 2], vec[i]))
		{
			swap(vec[i / 2], vec[i]);
			i /= 2;
		}
	}

	void percolate_down()
	{
		unsigned long long i = 1;
		while(i <= hsize)
		{
			if(i * 2 == hsize)
			{
				if(comp(vec[i * 2], vec[i]))
					swap(vec[i], vec[i * 2]);
				break;
			}
			else if(i * 2 + 1 <= hsize)
			{
				if(comp(vec[i * 2], vec[i * 2 + 1])) //i * 2 < i * 2 + 1
				{	
					if(!comp(vec[i * 2], vec[i])) break; // i * 2 >= i
					else
					{
						swap(vec[i * 2], vec[i]);
						i = i * 2;
					}
				}
				else 
				{
					if(!comp(vec[i * 2 + 1], vec[i])) break; // 
					else
					{
						swap(vec[i * 2 + 1], vec[i]);
						i = i * 2 + 1;
					}
				}
			}
			else break;
		}
	}

	T &top(){assert(!empty());return vec[1];};
	bool empty(){return hsize <= 0;};	
	
	void push(T &x)
	{
		hsize++;
		check_resize_(hsize);
		vec[hsize] = x;
		percolate_up();
	}

	void pop()
	{
		if(hsize > 1)
		{
			swap(vec[1], vec[hsize]);
			hsize--;
			percolate_down();
		}
		else if(hsize == 1)
			hsize--;
	}
	
	void update_top(T &x)
	{
		top() = x;
		percolate_down();
	}
};

template<class T>
class QuickStack
{
/*
 *	Actually, this class is not quick as all.
 *	Next: try to use VtxIndexFile to instead malloc.
 */
private:
	const unsigned STACK_ADD_SIZE = 2048;
	T *vec;
	unsigned ssize, top_p;
public:
	QuickStack(unsigned ssize = 4096):ssize(ssize), top_p(0), vec(NULL)
	{
		vec = (T*)malloc(ssize * sizeof(T));
	}
	
	~QuickStack(){free(vec);}
	
	void resize(unsigned newsize)
	{
		assert(newsize > top_p);
		ssize = newsize;
		vec = (T*)realloc((void*)vec, newsize * sizeof(T));
	}

	bool empty(){return top_p == 0;}

	T top(){return vec[top_p];}

	void push(T &x)
	{
		if(top_p + 1 >= ssize)
			resize(ssize + STACK_ADD_SIZE);

		top_p++;
		vec[top_p] = x;
	}
	
	void pop(){if(top_p > 0) top_p--;}

};


#endif
