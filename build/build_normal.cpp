#include "build.h"
#include "pathgraph.h"

using namespace std;


ID N = 0; // maxid

/*
 *	File operaton
 */

void read_dataset_adj_no_size(char *fname, RawFile<ID> &rfile)
{
	FILE *f = fopen(fname, "r");

	//Start read dataset
	cout << "Start read dataset "; get_time(false); cout << endl;
	ID from;
	long long maxlen = 10000000;
	char *s = (char*)malloc(maxlen);
	char delims[] = " \t\n";

	long long brcnt = 0;
	while (fgets(s, maxlen, f) != NULL)
	{
		if (s[0] == '#' || s[0] == '%' || s[0] == '\n')
			continue; //Comment

		char *t = strtok(s, delims);
		from = atoi(t);
		N = max(N, from);

		ID to;
		while ((t = strtok(NULL, delims)) != NULL)
		{
			to = atoi(t);
			N = max(N, to);
			
			RawUnit<ID> &ru = rfile[brcnt];
                        ru.first = from;
                        ru.second = to;
                        brcnt++;	
		}
		///*
		if (from % 100000 == 0)
			cout << from << endl;
		//*/
		/*
		if (++brcnt % 1000000 == 0)
		cout << brcnt << endl;
		//*/
	}
	rfile.set_raw_num(brcnt);
	fclose(f);
	free(s);
	cout << "End read dataset max id: " << N << " "; get_time(); cout << endl;
}


void read_dataset_hlist(char *fname, RawFile<ID> &rfile)
{
	read_dataset_adj_no_size(fname, rfile);
}

void convert_raw_to_adj(RawFile<ID> &raw_file, VtxsFile<ID> &adj_file, bool is_in_not_out = false, 
	ID* mapping = NULL)
{
	cnt_t raw_size = raw_file.size();

	cout << "raw_size " << raw_size << endl;

	ID last_from = (0 - 1); //define a non-exist id to last_from
	bool first_set = true;

	adj_file.create_start();

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
				adj_file.create_add_vtx_end();
			else first_set = false;

			last_from = from;
			
			adj_file.create_add_vtx_start(from);
		}

		adj_file.create_add_bor(to);
	}
	if(!first_set)
		adj_file.create_add_vtx_end();

	adj_file.create_end();
}

int main(int argc, char *argv[])
{
	if (argc != 3)
	{
		cout << "Usage ./build <dataset.txt> <to dir>" << endl;
		return 0;
	}


	string dir(argv[2]);	

	//Remove old files.
	cout << "Remove old files"; get_time(false);
	string cmd = string("rm -f ") + dir + string("/*.bin");
	system(cmd.c_str());
	get_time(); cout << endl;

	//Record start time
	time_t start_time = get_time(false);

	cout << "\n------------------Read Dataset---------------------" << endl;
	//Create unsorted raw file from dataset
	cout << "Dataset --> Normal RawFile " << endl; get_time(false);
	RawFile<ID> us_rfile("unsorted_raw_file.bin", dir);
	us_rfile.open();
	read_dataset_adj_no_size(argv[1], us_rfile);
	//N = 105896267; 
	cout << "Max id: " << N << endl;
	

	//Sort it
	cout << "Unsort Normal RawFile --> Sorted  RawFile(first unit order) " << endl; get_time(false);
	RawFile<ID> s_rfile("sorted_raw_file.bin", dir);
	s_rfile.open();
	Sorter::sort_raw(us_rfile, s_rfile, true);
	get_time(); cout << endl;
	us_rfile.close();

	//Generate normal adj file
	cout << "Sorted RawFile(first unit order) --> Normal AdjFile Out"; get_time(false);
	VtxsFile<ID> fvtxs_out("normal_vtxs_out.bin", "normal_vindex_out.bin", dir);
	fvtxs_out.open();
	fvtxs_out.set_max_id(N);
	convert_raw_to_adj(s_rfile, fvtxs_out, false);
	get_time(); cout << endl;
	fvtxs_out.close();

	//Sort it
	cout << "Unsort Normal RawFile --> Sorted  RawFile(second unit order) " << endl; get_time(false);
	us_rfile.open();
	Sorter::sort_raw(us_rfile, s_rfile, false);
	get_time(); cout << endl;
	us_rfile.close();
	
	//Generate normal adj file
	cout << "Sorted RawFile(second unit order) --> Normal AdjFile In "; get_time(false);
	VtxsFile<ID> fvtxs_in("normal_vtxs_in.bin", "normal_vindex_in.bin", dir);
	fvtxs_in.open();
	fvtxs_in.set_max_id(N);
	convert_raw_to_adj(s_rfile, fvtxs_in, true);
	get_time(); cout << endl;
	fvtxs_in.close();
	s_rfile.close();

	us_rfile.destroy();
	s_rfile.destroy();

	cout << "----------------------------------------------------" << endl;
	cout << "Build normal file finish. Expires: " << get_time(false) - start_time << " ####" << endl;
	return 0;
}

