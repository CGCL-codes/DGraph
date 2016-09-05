#ifndef PATHGRAPH_H
#define PATHGRAPH_H

#include "gfile.h"
#include "type.h"
#include <stack>
#include <algorithm>

#ifndef SCC_DAG_API_H
#include "build.h"
class PathGraph
{
private:
	static void alloc_id(ID id, VtxIndexFile<ID> &out_otn, VtxIndexFile<ID> &out_nto, ID &index_id, ID* mapping = NULL, int incr = 1)
	{
		if(mapping != NULL)
			id = mapping[id];
		if(out_otn[id] == ID_NULL)
		{
			out_otn[id] = index_id;
			out_nto[index_id] = id;
			index_id += incr;
		}
	}

public:
	static void generate_pg_map(vector<SCC_Vtx*> &scc_vtxs, VtxsFile<ID> &vtxs,
			VtxIndexFile<ID> &out_otn, VtxIndexFile<ID> &out_nto, ID max_id, ID *mapping = NULL)
	{	
		ID index_id = max_id;
		long long size = vtxs.size();
		for(long long i = size - 1; i >= 0; i--)
			out_otn[i] = ID_NULL;

		for(long long i = scc_vtxs.size() - 1; i >= 0; i--) //No problem here, alloc id backwards.
		{
			SCC_Vtx *scc = scc_vtxs[(ID)i];
			//vtx in scc->scc_group is reverse dfs order
			long long vsize = scc->scc_group_id.size();
			//cout << "scc-" << scc->id << endl;


			//add bor
			for(long long j = 0; j < vsize; j++)
			{
				VtxUnit<ID> *vu = vtxs[scc->scc_group_id[j]];
			//	cout << scc->scc_group[j]->id << endl;
				cnt_t bor_size = 0;
				bor_size = *(vu->cnt_p);
				alloc_id(*(vu->id_p), out_otn, out_nto, index_id, mapping, -1);
				for(int k = 0; k < bor_size; k++)
				{
					alloc_id(vu->bor[k], out_otn, out_nto, index_id, mapping, -1);
				}
				free(vu);
			}
			
		//	cout << endl;
		}
		
		//for(int i = 0; i < out_otn.size(); i++)
		//	cout << i << " -> " << out_otn[i] << endl;


		cout << "PathGraph from id: " << index_id + 1  << endl;
	}


private:
	static int id_greater(const void *a, const void *b){return *(int*)a - *(int*)b;};
	static int id_less(const void *a, const void *b){return *(int*)b - *(int*)a;};
};

#endif

template<class T>
class PGVtxBorFile : public MMapFile{
//Storage structure: 
//		[offset_back]
//		[0:[compress list]]
//		[1:[compress list]]
//		where [compress list] (use base 128 Varint to compress delta value between this and pre)
//			[compress list] = [bor_cnt_t len, bor[0], compressed[bor[1] - bor[0]], compressed[bor[2] - bor[1]], ...]
//
//		Use a DegreeCnt[] to decrease amout when only need to read the degree of the file
//
private:
	int unit_size = sizeof(T);
	offset_t off_back = 0;
	T last = 0;
	ID now_id = 0;

public:
	PGVtxBorFile(string name, string dir): MMapFile(string("bor_") + name, dir), degreeCnt(string("degree_") + name, dir){};

	VtxIndexFile<bor_cnt_t> degreeCnt;

	Status open()
	{
		if(MMapFile::open() == ERROR) return ERROR;
		off_back = *(offset_t*)get_file_start();
        if(off_back == 0)
        {   
            off_back = sizeof(offset_t); //new file
            *(offset_t*)get_file_start() = off_back;
		}   

		if(degreeCnt.open() == ERROR) return ERROR;
	    return OK; 
    };  

    Status close()
    {   
        *(offset_t*)get_file_start() = off_back;
		if(degreeCnt.close() == ERROR) return ERROR;
        return MMapFile::close() ;
    }

	addr_t get_vtx_base_addr(offset_t off)
	{
		if(off <= 0 || off >= off_back)
			return NULL;
		return map_data(off);
	}
	
	offset_t get_off_back(){return off_back;}

};

/*
 *  Here are 3 ways to decompress data.
 *  1. template class PGVtxBor_t: 
 *		Define a template class to replace bor_t. Re-define operator [] to access compress data atomatically.
 *  2. Macro PG_Foreach:
 *		Define a macro, user call this macro to access compress data sequentially.
 *		2.1 PG_get(): Decompress in C/C++ function.
 *		2.2 PG_asm_get(): Recomended! Decompress in assemble function(refer to pathgraph.s).
 */

//Way 1:
template<class T>
class PGVtxBor_t
{
private:
//	T* bor = NULL;
//	bool is_init = false;
    volatile char* root;
//	bor_cnt_t size;

	//To access data
	volatile T last = 0;
	volatile ID this_id = 0;
	volatile char* this_p = NULL;

///*
	//backward order
	T get(volatile char* &p)
	{
		T x = 0, c = 0;
		//de-compress Base 128 Varint
		for(int shift = 0; shift < sizeof(T) * 8; shift += 7)	
		{
			p++;
			c = (((T)(*p) & 0x7f) << shift);
			x |= c;
			if(((*p) & 0x80) == 0)
				break;
		}
		return x;
	}
	//*/
/*
   //forward order
	T get(char* &p)
	{
		T x = 0;
		do
		{
			p++;
			x <<= 7;
			x |= (T)(*p) & 0x7f;

		}while(*p & 0x80);
		return x;
	}
//*/
/*
	
	void init()
	{
		//size is a cnt before bor root
		size = *((bor_cnt_t*)root - 1);
		T last = 0;
		char* p = root - 1; 
		bor = new T[size];
		for(int i = 0; i < size; i++)
		{
			bor[i] = last + get(p);
			last = bor[i];
		}
	}
*/
public:
	PGVtxBor_t(T* root = NULL): root((char*)root), this_p(root == NULL ? NULL : (char*)root - 1){};
	~PGVtxBor_t(){};//delete bor;};

	addr_t operator = (addr_t addr)
	{
//		if(is_init)
//			delete bor;
//		is_init = false;
		this_id = 0;
		last = 0;
		this_p = (char*)addr - 1;
		return (addr_t)(root = (char*)addr);
	};

	T operator [](ID id)
	{
		/*
		if(is_init == false) //heuristic method 
		{
			init();
			is_init = true;
		}
		
		return bor[id];
		//*/
		if(id < this_id)
		{
			this_id = 0;
			last = 0;
			this_p = root - 1;
		}

		for(;this_id <= id; this_id++)
			last += get(this_p);
		return last;
	}

};

//Way 2.1:
/*
//backward order
inline bool PG_get(register char* &pt, ID &idval)
{
	register int shift = -7;
	register ID ret = 0;
	do
	{
		ret |= ((ID)(*(pt) & 0x7f) << (shift += 7));
	}while(*(pt++) & 0x80);

	idval += ret;
	return true;
}
//*/
///*
//forward order
inline bool PG_get(register char* &pt, ID &idval)
{
	ID ret = 0;
	char* p = pt;
	do
	{
		ret = (ret << 7) | (ID)(*(p) & 0x7f);
	}while(*(p++) & 0x80);
	idval += ret;
	pt = p;
	return true;
}
//*/

//Way 2.2:
extern "C" inline bool PG_asm_get(char **pt, char *end, unsigned int *idval);
#define PG_Foreach(bor, idval) \
	idval = 0;\
	char* _pg_pt = bor == NULL ? NULL : (char*)((bor_cnt_t*)bor + 1),\
		*_pg_end = bor == NULL ? NULL : _pg_pt + *(bor_cnt_t*)bor;\
	while(PG_asm_get(&_pg_pt, _pg_end, &idval))


#define PG_Foreach2(bor, idval) \
	idval = 0;\
	register char* _pg_pt = bor == NULL ? NULL : (char*)((bor_cnt_t*)bor + 1),\
		*_pg_end = bor == NULL ? NULL :_pg_pt + *(bor_cnt_t*)bor;\
	while(_pg_pt < _pg_end && PG_get(_pg_pt, idval))


template<class T = ID>
class PGVtxsFile 
{
protected:
    bor_cnt_t bor_cnt = 0;
    offset_t this_off = 0;
	ID now_id = 0;
public:
    VtxIndexFile<offset_t> index_file;
    PGVtxBorFile<T> bor_file; //The only difference between class VtxsFile.
    PGVtxsFile(string bor_name, string index_name, string dir): index_file(index_name, dir), bor_file(bor_name, dir){};

	Status open()
	{
		if(index_file.open() == OK && bor_file.open() == OK)
		     return OK;
		return ERROR;
	}
	
	Status close()
	{
	    if(index_file.close() == OK && bor_file.close() == OK)
	         return OK;
	     return ERROR;
	}

	cnt_t size(){return index_file.size();}
	
	addr_t operator [](ID id)
    {
		return get_vtx_base_addr(id);
    }

    addr_t get_vtx_base_addr(ID id)
    {
        return bor_file.get_vtx_base_addr(index_file[id]);
    }

	bor_cnt_t degree(ID id)
	{
		return bor_file.degreeCnt[id];
	}
};


class OutPGBorFile : public OutFile
{
private:
	offset_t off;
	string name, dir;
public:
	OutPGBorFile(string name, string dir): OutFile(name, dir), off(sizeof(offset_t)), 
					name(name), dir(dir)
	{
		write_unit(off);
	};

	offset_t add_bor(vector<ID> &bor, bor_cnt_t cnt)
	{
		if(cnt == 0)
			return 0;

		offset_t ret_off = off;

		//we compress delta value
		//qsort((void*)bor, cnt, sizeof(ID), id_greater); 
		//sort(bor, bor + cnt, id_greater);
		sort(bor.begin(), bor.end(), id_greater);
	

		vector<char> cv;
		ID last = 0, delta;
		for(bor_cnt_t i = 0; i < cnt; i++)
		{
			delta = bor[i] - last;
			last = bor[i];
			compress_in(delta, cv);
		}

		bor_cnt_t size = cv.size();
		write_unit(size);
		for(int i = 0; i < cv.size(); i++)
			write_unit(cv[i]);
		
		off += sizeof(size) + size;
		return ret_off;
	}

	Status set_offset()
	{
		flush();
		off_t currpos = ::lseek(fos, 0, SEEK_CUR);
		::lseek(fos, 0, SEEK_SET);
		write_unit(off);
		flush();
		::lseek(fos, currpos, SEEK_SET);
		return OK;
	}


	Status close()
	{
		set_offset();
		OutFile::close();
		return OK;
		//write off back
		/*
		MMapFile bor_file(name, dir);
		bor_file.open();
		*(offset_t*)bor_file.get_file_start() = off;
		bor_file.close();
		*/
	}

	
private:
	void compress_in(ID unit, vector<char> &cv)
	{
		char out[10]; // 64 / 3 = 9.14, so 10 buffer is enough
		int top = 0;
		
		while(unit > 127)
		{
			out[top++] = unit & 0x7f;
			unit >>= 7;
		}
		out[top++] = unit;

		while(top > 0)
			cv.push_back(out[--top] | (top == 0 ? 0 : 0x80));

	}

//	static int id_greater(const void *a, const void *b){return *(ID*)a - *(ID*)b;};
	static bool id_greater(ID a, ID b){return a < b;};

};

class OutPGVtxsFile
{
public:
	VtxIndexFile<offset_t> index;
	VtxIndexFile<bor_cnt_t> degree;
	OutPGBorFile bor_file;

	OutPGVtxsFile(string name, string dir): index(string("index_") + name, dir), 
											degree(string("degree_") + name, dir),
											bor_file(string("bor_") + name, dir){};

	Status open()
	{
		return index.open() && degree.open() ? OK : ERROR;
	}

	Status close()
	{
		if(index.close() && degree.close() && bor_file.close())
			return OK;
		else
		{
			cout << "close  ERROR!!!" << endl;
			assert(0);
			return ERROR;
		}

	}

	Status generate(VtxIndexFile<ID> &vid_order, 
				VtxIndexFile<ID> &id_to_vid, 
				VtxIndexFile<ID> &vid_to_id, 
				VtxsFile<ID> &normal_vtxs)
	{
		//pre set size to forbid many remmapping
		degree.set_size(vid_order.size());
		index.set_size(vid_order.size());

		vector<ID> bor(1);
		for(cnt_t i = 0; i < vid_order.size(); i++)
		{
			ID vid = vid_order[i];
		//	cout << i << "-" << vid << endl;
			VtxUnit<ID> *vu = normal_vtxs[vid_to_id[vid]];		
			bor_cnt_t size = *(vu->cnt_p);
			//ID *bor = new ID[size]();
			bor.resize(size);

			for(bor_cnt_t j = 0; j < *(vu->cnt_p); j++)
				bor[j] = id_to_vid[vu->bor[j]];
			
			degree[i] = size;
			index[i] = bor_file.add_bor(bor, size);
			
			//delete[] bor;
			delete vu;
		}
		return OK;
	}


};






#endif
