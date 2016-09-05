#include "sorter.h"

using namespace std;

unsigned long long Sorter::PART_SIZE = DEFAULT_PART_SIZE;

class SortRawUnit
{
public:
	int part_id;
	unsigned long long off;	
	RawUnit<ID> ru;
	SortRawUnit(int part_id, unsigned long long off, ID from, ID to): part_id(part_id), off(off)
	{
	 	ru.first = from; 
		ru.second = to;
	};
	void set_ru(RawUnit<ID> &x){ru = x;};
};


unsigned long long part_num;
vector<unsigned long long> nelems;

bool sort_raw_unit_comp12(SortRawUnit* &a, SortRawUnit* &b)
{
	if(a->ru.first == b->ru.first)
		return a->ru.second < b->ru.second;
	return a->ru.first < b->ru.first;
}

bool sort_raw_unit_comp21(SortRawUnit* &a, SortRawUnit* &b)
{
	if(a->ru.second == b->ru.second)
		return a->ru.first < b->ru.first;
	return a->ru.second < b->ru.second;
}

bool id_raw_comp12(const RawUnit<ID> &a, const RawUnit<ID> &b)
{
	if(a.first == b.first)
		return a.second < b.second;
	return a.first < b.first;
}

bool id_raw_comp21(const RawUnit<ID> &a, const RawUnit<ID> &b)
{
	if(a.second == b.second)
		return a.first < b.first;
	return a.second < b.second;
}

void merge_part1(RawFile<ID> &in_raw, OutRawFile<ID> &outfile, bool (*compare)(SortRawUnit* &, SortRawUnit* &))
{

	cout << "Enter merge part part_num: " << part_num << endl;
	//Inistralize pque
	UpdateTopHeap<SortRawUnit*> pque(compare);
	for(int i = 0; i < part_num; i++)
	{
		RawUnit<ID> &ru = in_raw[i * Sorter::PART_SIZE];
		SortRawUnit *sru = new SortRawUnit(i, 0, ru.first, ru.second);
		pque.push(sru);
		nelems[i]--;
	}

	int line_cnt = 0;
	while(!pque.empty())
	{
		SortRawUnit *sru = pque.top();	

		//RawUnit<ID> &ru = out_raw[line_cnt++];
		//ru = sru->ru;
		line_cnt++;
		outfile.write_raw(sru->ru.first, sru->ru.second);

		if(nelems[sru->part_id] > 0)
		{
			RawUnit<ID> &ru = in_raw[sru->part_id * Sorter::PART_SIZE + sru->off + 1];
			sru->set_ru(ru);
			sru->off++;
			pque.percolate_down();
			nelems[sru->part_id]--;
		}
		else		
		{
			pque.pop();
			delete sru;
		}
	}
	//out_raw.set_raw_num(line_cnt);
}

void Sorter::sort_raw1(RawFile<ID> &in_raw, RawFile<ID> &out_raw, bool is_by_first, unsigned long long part_size)
{
	//In order to narrow memeory occupation, we use OutRawFile to write RawFile
	out_raw.close();
	OutRawFile<ID> outfile(out_raw.name, out_raw.dir);

	cout << "big data sort" << endl;
	//set part size
	PART_SIZE = part_size;

	unsigned long long raw_size = in_raw.size();
	if(raw_size == 0)
		return;

	//int (*compare)(const void*, const void*) = is_by_first ? (RawFile<ID>::vcompare12) : (RawFile<ID>::vcompare21);
	bool (*compare)(const RawUnit<ID> &, const RawUnit<ID> &) = is_by_first ? (id_raw_comp12) : (id_raw_comp21);

	part_num = (raw_size - 1)/ PART_SIZE + 1;

	//Sort each part
	nelems.clear();
	nelems.resize(part_num, 0);

	cout << "start sort part" << endl;
#pragma omp parallel for
	for(int i = 0; i < part_num; i++)
	{
		void *from = (void*)(in_raw.begin() + i * PART_SIZE);
		unsigned long long nelem = min(PART_SIZE, raw_size - i * PART_SIZE),
		    width = sizeof(RawUnit<ID>);
		nelems[i] = nelem;

		//qsort(from, nelem, width, compare);
		sort((RawUnit<ID>*)from, (RawUnit<ID>*)from + nelem, compare);
	}

	cout << "Start merge part" << endl;
	//Merge parts using heap.	
	bool (*merge_cmp)(SortRawUnit* &, SortRawUnit* &) 
			= is_by_first ? &sort_raw_unit_comp12 : &sort_raw_unit_comp21;
	merge_part1(in_raw, outfile, merge_cmp);
	outfile.close();
	out_raw.open();
/*
	cout << "in_raw" << endl;
	for(int i = 0; i < raw_size; i++)
		cout << in_raw[i].first << "\t:\t" << in_raw[i].second << endl;
	
	cout << "out_raw" << endl;
	for(int i = 0; i < raw_size; i++)
		cout << out_raw[i].first << "\t:\t" << out_raw[i].second << endl;
//*/
}

vector<unsigned long long> elems, froms_off;

void merge_part2(RawFile<ID> &in_raw, OutRawFile<ID> &outfile, bool (*compare)(SortRawUnit* &, SortRawUnit* &), int tnum)
{
	cout << "Enter merge part part_num: " << tnum  << endl;
	//Inistralize pque
	UpdateTopHeap<SortRawUnit*> pque(compare);
	for(int i = 0; i < tnum; i++)
	{
		RawUnit<ID> &ru = in_raw[froms_off[i]];
		SortRawUnit *sru = new SortRawUnit(i, 0, ru.first, ru.second);
		pque.push(sru);
		elems[i]--;
	}

	int line_cnt = 0;
	while(!pque.empty())
	{
		SortRawUnit *sru = pque.top();	

		//RawUnit<ID> &ru = out_raw[line_cnt++];
		//ru = sru->ru;
		line_cnt++;
		outfile.write_raw(sru->ru.first, sru->ru.second);

		if(elems[sru->part_id] > 0)
		{
			RawUnit<ID> &ru = in_raw[froms_off[sru->part_id] + sru->off + 1];
			sru->set_ru(ru);
			sru->off++;
			pque.percolate_down();
			elems[sru->part_id]--;
		}
		else		
		{
			pque.pop();
			delete sru;
		}
	}
}

void Sorter::sort_raw2(RawFile<ID> &in_raw, RawFile<ID> &out_raw, bool is_by_first, int tnum)
{
	//In order to narrow memeory occupation, we use OutRawFile to write RawFile
	out_raw.close();
	string name = out_raw.name, dir = out_raw.dir;
	OutRawFile<ID> outfile(name, dir);
	long long raw_size = in_raw.size();
	tnum = min(tnum, raw_size);

	cout << "small data sort " << tnum << endl;

	if(raw_size == 0)
		return;

    //int (*compare)(const void*, const void*) = is_by_first ? (RawFile<ID>::vcompare12) : (RawFile<ID>::vcompare21);
	bool (*compare)(const RawUnit<ID> &, const RawUnit<ID> &) = is_by_first ? (id_raw_comp12) : (id_raw_comp21);

	elems.clear();
	froms_off.clear();

	elems.resize(tnum, 0);
	froms_off.resize(tnum, 0);

	unsigned long long from = 0;
	for(int i = 0; i < tnum; i++)
	{
		unsigned long long nelem = raw_size / tnum + (i < (raw_size % tnum));
		elems[i] = nelem;
		froms_off[i] = from;
		from += nelem;
	}

	cout << "start sort part" << endl;

	#pragma omp parallel for schedule(static)
	for(int i = 0; i < tnum; i++)	
	{
		RawUnit<ID> *begin = (RawUnit<ID>*)(in_raw.begin() + froms_off[i]),
					*end = begin + elems[i];
		sort(begin, end, compare);
	}
		//qsort((void*)(in_raw.begin() + froms_off[i]), elems[i], sizeof(RawUnit<ID>), compare);


	cout << "Start merge part" << endl;
	//Merge parts using heap.	
	bool (*merge_cmp)(SortRawUnit* &, SortRawUnit* &) 
			= is_by_first ? &sort_raw_unit_comp12 : &sort_raw_unit_comp21;
	merge_part2(in_raw, outfile, merge_cmp, tnum);
	outfile.close();
	out_raw.open();
}








