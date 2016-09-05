#ifndef G_FILE
#define G_FILE

#include "mmap_file.h"
#include "type.h"
#include <assert.h>


using namespace std;

template<class T>
T &max(T &a, T &b){return a > b ? a : b;};

template<class T> //offset_t or ID -> T
struct VtxUnit{
	ID *id_p = NULL;
	bor_cnt_t *cnt_p;
	T *bor;
};

template <class T>
class ZeroUnit
{
public:
	static VtxUnit<T>* ZeroVtxUnit(ID id)
	{
		VtxUnit<T> *vu = new VtxUnit<T>();
		vu->id_p = new ID(id);
		vu->cnt_p = new bor_cnt_t(0);
		vu->bor = NULL;
		return vu;
	}
};


/*
 *	VtxUnitData: Quick access vtx data using offset, instead of creating many VtxUnit;
 */
static ID zero_id = 0;
static bor_cnt_t zero_cnt = 0;

template <class T>
class VtxUnitData
{
public:
	static ID &id(addr_t base){return base == NULL ? zero_id : *(ID*)base;}
	static ID &id(addr_t base1, addr_t base2){return base1 == NULL ? id(base2) : id(base1);}
	static bor_cnt_t &bor_cnt(addr_t base){return base == NULL ? zero_cnt : *(bor_cnt_t*)(base + sizeof(ID));}
	static T* bor(addr_t base){return base == NULL ? NULL : (T*)(base + sizeof(ID) + sizeof(bor_cnt_t));}
};


class VtxUnitIdData : public VtxUnitData<ID>{};
class VtxUnitOffData : public VtxUnitData<offset_t>{};

template<class T>
class VtxIndexFile : public MMapFile{

//Storage structure: [cnt_t size] [T unit0] [T unit1] [T unit2] [T unit3] ...

private:
	cnt_t max_size;
	long unsigned int unit_size = sizeof(T), cnt_size = sizeof(T) > sizeof(cnt_t) ? sizeof(T) : sizeof(cnt_t);
public:
	VtxIndexFile(string name, string dir): MMapFile(name, dir), max_size(0){};

	Status open()
	{
		if(MMapFile::open() == ERROR) return ERROR;
		max_size = *(cnt_t*)get_file_start();
		return OK;
	};


	Status close()
	{
		*(cnt_t*)get_file_start() = max_size;
		return MMapFile::close();
	}

	T & operator [](ID id)
	{
		if(max_size < id + 1)
		{
			max_size = (cnt_t)(id + 1);
		}
		offset_t off = cnt_size + id * unit_size; //for alignment, not use: sizeof(cnt_t) + id * unit_size;
		assert(expand(off + unit_size) == OK);
		addr_t addr = map_data(off);
		return (*(T*)addr);
	};
	
	void set_size(cnt_t size)
	{
		if(size == 0)
			return;
		this[size - 1];
	}

	cnt_t size(){return max_size;};
	ID *get_start(){return (ID*)(get_file_start() + cnt_size);}
};

template<class T>
class VtxBorFile : public MMapFile{

//Storage structure: 	[offset_back]
//			[0:[ID id] [bor_cnt_t cnt] [T unit1] [T unit2] ...] 
//		     	[1:[ID id] ...] ...

private:
	int unit_size = sizeof(T);
	//offset_t* &off_back_p = (offset_t*)get_file_start();
	offset_t off_back = 0;
	bool is_create_mode = false;
public:
	VtxBorFile(string dir, string name):MMapFile(dir, name){};

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
		assert(!is_create_mode);

		*(offset_t*)get_file_start() = off_back;
		return MMapFile::close();
	}

	VtxUnit<T>* get_vtx(offset_t off) //Remenber to delete vu;
	{
		assert(!is_create_mode);

		if(off == 0)
			return ZeroUnit<T>::ZeroVtxUnit(ID(0) - 1);//return  max id that is not existed.

		if(off >= off_back)
			return NULL;

		VtxUnit<T> *vu = new VtxUnit<T>();
		vu->id_p = (ID*)map_data(off, sizeof(ID));
		vu->cnt_p = (bor_cnt_t*)map_data(off + sizeof(ID), sizeof(bor_cnt_t));
		vu->bor = (T*)map_data(off + sizeof(ID) + sizeof(bor_cnt_t)); //no need to set size.
		return vu;
	}

	addr_t get_vtx_base_addr(offset_t off)
	{
		assert(!is_create_mode);

		if(off <= 0 || off >= off_back)
			return NULL;
		
		return map_data(off);
		//return get_file_start() + off;
	}

	offset_t add_vtx(ID id, bor_cnt_t cnt)
	{
		assert(!is_create_mode);

		offset_t off = off_back;
		*(ID*)map_data(off, sizeof(ID)) = id;
		*(bor_cnt_t*)map_data(off + sizeof(ID), sizeof(bor_cnt_t)) = cnt;
		off_back = off + sizeof(ID) + sizeof(bor_cnt_t);
		return off;
	}
	
	//First, use add_vtx to set vertex, and use add_bor to add border.
	Status add_bor(T unit)
	{
		assert(!is_create_mode);

		T* data_p = (T*)map_data(off_back, sizeof(T));
		if(data_p == NULL) return ERROR;

		*data_p = unit;
		off_back += unit_size;
		return OK;
	}

/*
 *	Create Mode is design for low memory. Write data to file directly instead of mmapping it in memory and writing.
 */
	OutFile *out_file;
	offset_t out_off = sizeof(offset_t);
	vector<T> out_bor;

	Status create_start()
	{
		assert(!is_create_mode);

		if(close() == ERROR) return ERROR;
		
		out_file = new OutFile(MMapFile::name, MMapFile::dir);
		out_off = sizeof(offset_t);
		is_create_mode = true;		
		out_file->write_unit(out_off); //write offset space

		return OK;
	}

	offset_t create_add_vtx_start(ID id)
	{
		assert(out_bor.size() == 0);

		offset_t off = out_off;
		out_file->write_unit(id);
		out_off += sizeof(ID);
		return off;
	}

	Status create_add_bor(T unit)
	{
		out_bor.push_back(unit);
	}

	Status create_add_vtx_end()
	{
		bor_cnt_t size = out_bor.size();
		out_file->write_unit(size);
		for(bor_cnt_t i = 0; i < size; i++)	
			out_file->write_unit(out_bor[i]);

		out_off += sizeof(bor_cnt_t) + size * sizeof(T);
		out_bor.clear();
		return OK;
	}

	Status create_end()
	{
		assert(is_create_mode);
		out_file->close();
		open();
	    *(offset_t*)get_file_start() = out_off;
		off_back = out_off;
		is_create_mode = false;
	    return OK;
	}

};

template<class T = ID>
class VtxsFile
{
protected:
	bor_cnt_t bor_cnt = 0;
	offset_t this_off = 0;
public:
	VtxIndexFile<offset_t> index_file;
	VtxBorFile<T> bor_file;
	VtxsFile(string bor_name, string index_name, string dir): index_file(index_name, dir), bor_file(bor_name, dir){};

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

	Status destroy()
	{
		return index_file.destroy() && bor_file.destroy() ? OK : ERROR;
	}

	cnt_t size(){return index_file.size();}

	VtxUnit<T>* operator [](ID id) //Remember to delete VtxUnit*;
	{
		offset_t off = index_file[id];
		if(off == 0) return ZeroUnit<T>::ZeroVtxUnit(id);
		else return	bor_file.get_vtx(off); 
	}

	Status add_vtx_start(ID id, bor_cnt_t cnt = 0)
	{
		this_off = bor_file.add_vtx(id, cnt);
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
		*(bor_file.get_vtx(this_off)->cnt_p) = bor_cnt;
		return OK;
	}

	Status set_max_id(ID id)
	{
		index_file[id]; //expand and set size atomatically
		return OK;
	}


	//To quick access base addr of vtx
	addr_t get_vtx_base_addr(ID id)
	{
		return bor_file.get_vtx_base_addr(index_file[id]);
	}

	Status create_start()
	{
		return bor_file.create_start();
	};

	Status create_add_vtx_start(ID id)
	{
		this_off = bor_file.create_add_vtx_start(id);
		index_file[id] = this_off;
		bor_cnt = 0;
		return OK;
	}

	Status create_add_bor(T unit)
	{
		bor_cnt++;
		return bor_file.create_add_bor(unit);
	}

	Status create_add_vtx_end()
	{
		return bor_file.create_add_vtx_end();
	}

	Status create_end()
	{
		return bor_file.create_end();
	}

};

template<class Ta, class Tb = Ta>
struct RawUnit
{
	Ta first;
	Tb second;
};

template<class Ta, class Tb = Ta>
class RawFile : public MMapFile
{
/*
 *	Storage structure: [cnt_t size] [[a0], [b0]] [[a1], [b1]] ...
 *
 *	Notice:
 *	In the case of small file (no more than twitter-2010.txt),
 *	using this class to write file is faster than OutRawFile class.
 *	For big dataset, use class OutRawFile instead.
 */

private:
	addr_t &raw_num_p = get_file_start();
public:
	RawFile(string name, string dir): MMapFile(name, dir){};

	Status open()
	{
		if(MMapFile::open() == OK && expand(sizeof(cnt_t) - 1) == OK)
			return OK;
		return ERROR;
	}
	
	RawUnit<Ta, Tb> & operator [](cnt_t raw_id) 
	{
		assert(expand(sizeof(cnt_t) + (raw_id + 1) * (sizeof(Ta) + sizeof(Tb))) == OK);
		
		addr_t addr = map_data(sizeof(cnt_t) + raw_id * (sizeof(Ta) + sizeof(Tb)), sizeof(Ta) + sizeof(Tb));

		return *(RawUnit<Ta, Tb>*)addr;
	}
	
	Status set_raw_num(cnt_t size)
	{
		if(!is_opened)
		{
			cout << "set_raw_num: file is not opened." << endl;
			return ERROR; 
		}
		*(cnt_t*)raw_num_p = size;
		return OK;
	}

	cnt_t size(){return *(cnt_t*)raw_num_p;}
	
	RawUnit<Ta, Tb>* begin()
	{
		cnt_t s = size();
		if(s == 0) return NULL;
		else return (RawUnit<Ta, Tb>*)(get_file_start() + sizeof(cnt_t));
	}

	RawUnit<Ta, Tb>* end()
	{
		cnt_t s = size();
		if(s == 0) return NULL;
		else return (RawUnit<Ta, Tb>*)(get_file_start() + sizeof(cnt_t) + sizeof(RawUnit<Ta, Tb>)* s);
	}

	static int compare12(RawUnit<Ta, Tb> &rua, RawUnit<Ta, Tb> &rub)
	{
		if(rua.first == rub.first)
			return rua.second - rub.second;
		return rua.first - rub.first;
	}

	
	static int compare21(RawUnit<Ta, Tb> &rua, RawUnit<Ta, Tb> &rub)
	{
		if(rua.second == rub.second)
			return rua.first - rub.first;
		return rua.second - rub.second;
	}
	
	static int vcompare12(const void *a, const void *b)
	{
		return compare12(*(RawUnit<Ta, Tb>*)a, *(RawUnit<Ta, Tb>*)b);
	}
	
	static int vcompare21(const void *a, const void *b)
	{
		return compare21(*(RawUnit<Ta, Tb>*)a, *(RawUnit<Ta, Tb>*)b);
	}
};

template<class Ta, class Tb = Ta> //data type
class OutRawFile :  public OutFile
{

//Storage structure: [cnt_t size] [[a0], [b0]] [[a1], [b1]] ...

private:
	cnt_t raw_num;
public:
	OutRawFile(string name, string dir)
		:OutFile(name, dir), raw_num(0)
	{
		//create a space for raw_num
		write_unit(raw_num);
	};

	
	
	//template<class Ta, class Tb>
	Status write_raw(Ta a, Tb b)
	{
		Status sa = write_unit(a), sb = write_unit(b), 
		rs = (sa == OK && sb == OK) ? OK : ERROR;
		
		if(rs == OK) raw_num++;
		
		return rs;
	}

	Status close()
	{
		OutFile::close();
		MMapFile mf(name, dir);
		mf.open();
		cnt_t *raw_num_p = (cnt_t*)mf.map_data(0, sizeof(cnt_t));
		*raw_num_p = raw_num;
		mf.close();
		return OK;
	}
};

#endif

