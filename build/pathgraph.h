#ifndef PATHGRAPH_H
#define PATHGRAPH_H

#include "gfile.h"
#include "type.h"
#include <stack>

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

	static void generate_map(vector<SCC_Vtx*> &scc_vtxs, VtxsFile<ID> &vtxs, VtxIndexFile<ID> &map_otn,
			VtxIndexFile<ID> &out_otn, VtxIndexFile<ID> &out_nto, 
			bool is_in_not_out, ID *mapping = NULL)
	{	
		ID index_id = 0;
		long long size = vtxs.size();
		for(long long i = size - 1; i >= 0; i--)
			out_otn[i] = ID_NULL;

		for(ID i = 0; i < scc_vtxs.size(); i++)
		{
			SCC_Vtx *scc = scc_vtxs[i];
			//vtx in scc->scc_group is reverse dfs order
			long long vsize = scc->scc_group.size();
			//cout << "scc-" << scc->id << endl;

			//if is out, add it self before it's bor
			if(vsize == 1  && scc->const_incnt == 0 && !is_in_not_out)
                        {
                                alloc_id(scc->scc_group[0]->id, out_otn, out_nto, index_id);
                          //      cout << scc->scc_group[0]->id << " -> " << out[scc->scc_group[0]->id] << endl;
                        }

			//add bor
			for(long long j = vsize - 1; j >= 0; j--)
			{
				VtxUnit<ID> *vu = vtxs[map_otn[scc->scc_group[j]->id]];
			//	cout << scc->scc_group[j]->id << endl;
				cnt_t bor_size = 0;
				bor_size = *(vu->cnt_p);

				for(int k = 0; k < bor_size; k++)
				{
					alloc_id(vu->bor[k], out_otn, out_nto, index_id, mapping);
				//	cout << mapping[vu->bor[k]] << " -> " << out[mapping[vu->bor[k]]] << endl;
				}
				free(vu);
			}
			
			//if is in, add it self after it's bor
			if(vsize == 1 && scc->outcnt == 0 && is_in_not_out)
			{
				alloc_id(scc->scc_group[0]->id, out_otn, out_nto, index_id);
			//	cout << scc->scc_group[0]->id << " -> " << out[scc->scc_group[0]->id] << endl;	
			}
		//	cout << endl;
		}	
		cout << "PathGraph id size: " << index_id  << endl;
	}


	static void generate_map_os(vector<SCC_Vtx*> &scc_vtxs, VtxsFile<ID> &vtxs, VtxIndexFile<ID> &map_otn,
			VtxIndexFile<ID> &out_otn, VtxIndexFile<ID> &out_nto, ID max_id, ID *mapping = NULL)
	{	
		ID index_id = max_id;
		long long size = vtxs.size();
		for(long long i = size - 1; i >= 0; i--)
			out_otn[i] = ID_NULL;

		for(long long i = scc_vtxs.size() - 1; i >= 0; i--)
		{
			SCC_Vtx *scc = scc_vtxs[(ID)i];
			//vtx in scc->scc_group is reverse dfs order
			long long vsize = scc->scc_group.size();
			//cout << "scc-" << scc->id << endl;


			//add bor
			for(long long j = 0; j < vsize; j++)
			{
				VtxUnit<ID> *vu = vtxs[map_otn[scc->scc_group[j]->id]];
			//	cout << scc->scc_group[j]->id << endl;
				cnt_t bor_size = 0;
				bor_size = *(vu->cnt_p);
				alloc_id(*(vu->id_p), out_otn, out_nto, index_id, mapping, -1);
				for(int k = 0; k < bor_size; k++)
				{
					alloc_id(vu->bor[k], out_otn, out_nto, index_id, mapping, -1);
				//	cout << mapping[vu->bor[k]] << " -> " << out[mapping[vu->bor[k]]] << endl;
				}
				free(vu);
			}
			
		//	cout << endl;
		}	
		cout << "PathGraph from id: " << index_id + 1  << endl;
	}

	static void convert_adj_file(VtxsFile<ID> &vtxs, VtxIndexFile<ID> &pg_map, ID *mapping = NULL)
	{
		for(ID i = 0; i < vtxs.size(); i++)
		{
			VtxUnit<ID> *vu = vtxs[i];
			cnt_t size = *(vu->cnt_p);
			
			*(vu->id_p) = pg_map[mapping[*(vu->id_p)]];
			for(ID j = 0; j < size; j++)
				vu->bor[j] = pg_map[mapping[vu->bor[j]]];

			//Sort to PathGraph order
			qsort((void*)vu->bor, *(vu->cnt_p), sizeof(ID), id_greater);
		}
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
//		[0:[ID id] [bor_cnt_t cnt] [compress list]]
//		[1:[ID id] [bor_cnt_t cnt] [compress list]]
//		where [compress list] (use base 128 Varint to compress delta value between this and pre)
//			[compress list] = [bor_cnt_t len, bor[0], compressed[bor[1] - bor[0]], compressed[bor[2] - bor[1]], ...]
//
private:
	int unit_size = sizeof(T);
	offset_t off_back = 0;
	T last = 0;
public:
	PGVtxBorFile(string dir, string name):MMapFile(dir, name){};

	Status open()
	{
		if(MMapFile::open() == ERROR) return ERROR;
		off_back = *(offset_t*)get_file_start();
        if(off_back == 0)
        {   
            off_back = sizeof(offset_t); //new file
            *(offset_t*)get_file_start() = off_back;
		}   
	    return OK; 
    };  

    Status close()
    {   
        *(offset_t*)get_file_start() = off_back;
        return MMapFile::close();
    }

	addr_t get_vtx_base_addr(offset_t off)
	{
		if(off <= 0 || off >= off_back)
			return NULL;
		return map_data(off);
	}

	bor_cnt_t* get_vtx_bor_size_addr(offset_t off)
	{
		return (bor_cnt_t*)map_data(off + sizeof(ID), sizeof(bor_cnt_t));
	}
	bor_cnt_t* get_vtx_bor_compress_len_addr(offset_t off)
	{
		return (bor_cnt_t*)map_data(off + sizeof(ID) + sizeof(bor_cnt_t), sizeof(bor_cnt_t));
	}
	
	offset_t get_off_back(){return off_back;}

	offset_t add_vtx(ID id, bor_cnt_t cnt)
	{
		last = 0;
		offset_t off = off_back;
        *(ID*)map_data(off, sizeof(ID)) = id; 
        *(bor_cnt_t*)map_data(off + sizeof(ID), sizeof(bor_cnt_t)) = cnt;
		*(bor_cnt_t*)map_data(off + sizeof(ID) + sizeof(bor_cnt_t), sizeof(bor_cnt_t)); //we assume that len is bor_cnt_t
        off_back = off + sizeof(ID) + sizeof(bor_cnt_t) * 2;
        return off;
	}

	Status add_bor(T unit)
	{
		T delta = unit - last;
		//compress delta value using: Base 128 Varint
		char* data_p;
///*
		//forward order
		char out[10];
		int top = 0;
		while(delta > 127)
		{
			out[top++] = delta & 0x7f;
			delta >>= 7;
		}
		out[top++] = delta;

		while(top > 0)
		{
			data_p = (char*)map_data(off_back, sizeof(char));
			*data_p = out[--top] | (top == 0 ? 0 : 0x80);
			off_back += sizeof(char);
		}
		last = unit;
//*/
/*
		//backward order
		while(delta > 127) //unit should be unsigned data
		{
			data_p = (char*)map_data(off_back, sizeof(char));
			(*data_p) = (char)(0x80 | delta & 0x7f);
			delta >>= 7;
			off_back += sizeof(char);
		}
		data_p = (char*)map_data(off_back, sizeof(char));
		(*data_p) = (char)delta;
		off_back += sizeof(char);
		last = unit;
		//*/
		return OK;
	}

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
	char* _pg_pt = (char*)((bor_cnt_t*)bor + 1),\
		*_pg_end = _pg_pt + *(bor_cnt_t*)bor;\
	while(PG_asm_get(&_pg_pt, _pg_end, &idval))


#define PG_Foreach2(bor, idval) \
	idval = 0;\
	register char* _pg_pt = (char*)((bor_cnt_t*)bor + 1),\
		*_pg_end = _pg_pt + *(bor_cnt_t*)bor;\
	while(_pg_pt < _pg_end && PG_get(_pg_pt, idval))


template<class T = ID>
class PGVtxsFile 
{
protected:
    bor_cnt_t bor_cnt = 0;
    offset_t this_off = 0;
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

    Status add_vtx_start(ID id, ID virtual_id, bor_cnt_t cnt = 0)
    {
        this_off = bor_file.add_vtx(virtual_id, cnt);
        index_file[id] = this_off;
        bor_cnt = 0;
        return OK;
    }
		    
    Status add_bor(T unit)
    {
        bor_cnt++;
        return bor_file.add_bor(unit);
    }

    Status add_vtx_end()
    {
        *(bor_file.get_vtx_bor_size_addr(this_off)) = bor_cnt;
		*(bor_file.get_vtx_bor_compress_len_addr(this_off)) = 
			(bor_cnt_t)(bor_file.get_off_back() - this_off - sizeof(ID) - sizeof(bor_cnt_t) * 2);
        return OK;
    }

    addr_t get_vtx_base_addr(ID id)
    {
        return bor_file.get_vtx_base_addr(index_file[id]);
    }

	Status generate(VtxsFile<T> &vtxs)
	{
		for(ID i = 0; i < vtxs.size(); i++)
		{
			VtxUnit<T> *vu = vtxs[i];
			add_vtx_start(i, *(vu->id_p));
			for(ID j = 0; j < *(vu->cnt_p); j++)
			{
				add_bor(vu->bor[j]);
			}
			add_vtx_end();
			delete vu;
		}
		return OK;
	}

};






#endif
