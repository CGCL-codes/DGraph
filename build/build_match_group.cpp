/*
 *	Build match group data for testing.
 *	Generate normal_vtxs_in.bin and normal_vindex_in.bin
 */

#include "build.h"
#include "pathgraph.h"

using namespace std;

/*
 *	File operaton
 */

void convert_raw_to_adj(RawFile<ID> &raw_file, VtxsFile<ID> &adj_file, bool is_in_not_out = false, 
	ID* mapping = NULL)
{
	cnt_t raw_size = raw_file.size();

	ID last_from = (0 - 1); //define a non-exist id to last_from
	bool first_set = true;

	for(cnt_t i = 0; i < raw_size; i++)
	{
		RawUnit<ID> &ru = raw_file[i];
		ID from, to;
		
		//set from to
		if(!is_in_not_out)
		{
			from = ru.first;
			to = ru.second;
		}
		else
		{
			from = ru.second;
			to = ru.first;
		}
		
		if(mapping != NULL)
		{
			from = mapping[from];
			to = mapping[to];
		}

		if(last_from != from)
		{
			if(!first_set) 
				adj_file.add_vtx_end();
			else first_set = false;

			last_from = from;
			
			adj_file.add_vtx_start(from);
		}

		adj_file.add_bor(to);
	}
	if(!first_set)
		adj_file.add_vtx_end();
}

void convert_raw_file(RawFile<ID> &rfile, ID* mapping)
{
	#pragma omp parallel for schedule(static)
        for(int i = 0; i < rfile.size(); i++)
        {
                RawUnit<ID> &ru = rfile[i];
                ru.first = mapping[ru.first];
                ru.second = mapping[ru.second];
        }
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		cout << "Usage ./build_match_group <data dir>" << endl;
		return 0;
	}


	string dir(argv[1]);	

	//Remove old files.
	cout << "Remove old files"; get_time(false);
	string cmd = string("rm -f ") + dir + string("/normal_vindex_in.bin");
	system(cmd.c_str());
	cmd = string("rm -f ") + dir + string("/normal_vtx_in.bin");
	system(cmd.c_str());
	get_time(); cout << endl;

	//Record start time
	time_t start_time = get_time(false);

	cout << "\n------------------Build Match Group---------------------" << endl;
	cout << "Normal Unsorted RawFile(new) --> Normal Unsorted RawFile(old) " << endl; get_time(false);
	RawFile<ID> us_rfile("unsorted_raw_file.bin", dir);
	us_rfile.open();
	//Unsorted_raw_file has been convert to new id. Thus, we need to convert it back.
	VtxIndexFile<ID> vtxs_map_nto("vtxs_map_new_to_old.bin", dir);
	vtxs_map_nto.open();	
	convert_raw_file(us_rfile, vtxs_map_nto.get_start());
	vtxs_map_nto.close();
	get_time(); cout << endl;

	//Sort it
	cout << "Sort Normal RawFile --> Sorted  RawFile(second unit order) " << endl; get_time(false);
	RawFile<ID> s_rfile("sorted_raw_file.bin", dir);
	s_rfile.open();
	Sorter::sort_raw(us_rfile, s_rfile, false);
	get_time(); cout << endl;

	//Generate normal adj file
	cout << "Sorted RawFile(second unit order) --> Normal AdjFile in"; get_time(false);
	VtxsFile<ID> fvtxs_in("normal_vtxs_in.bin", "normal_vindex_in.bin", dir);
	fvtxs_in.open();
	convert_raw_to_adj(s_rfile, fvtxs_in, true);
	get_time(); cout << endl;

	s_rfile.close();
	fvtxs_in.close();

	//Convert unsorted_raw_file back
	cout << "Convert back: Normal Unsorted RawFile(old) --> Normal Unsorted RawFile(new) " << endl; get_time(false);
	VtxIndexFile<ID> vtxs_map_otn("vtxs_map_old_to_new.bin", dir);
	vtxs_map_otn.open();	
	convert_raw_file(us_rfile, vtxs_map_otn.get_start());
	vtxs_map_otn.close();
	get_time(); cout << endl;
	
	us_rfile.close();
	cout << "---------------------------------------------------" << endl;
	cout << "Build end." << " cost: " << get_time(false) - start_time << " ms" << endl;
	return 0;
}
