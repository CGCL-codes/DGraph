/*
 *	Generate scc data: scc id, vtxs in scc, scc in, scc out
 */
#include "gfile.h"
#include <string>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

int main(int argc, char *argv[])
{
    	if (argc != 2)
    	{	
        	cout << "Usage ./get_topo_top <data dir>" << endl;
        	return 0;
    	}

	VtxIndexFile<ID> map_nto("pg_map_matrixos_nto.bin", argv[1]);	
	map_nto.open();
	RawFile<ID> levels("scc_topo_level.bin", argv[1]);
	levels.open();
	VtxsFile<ID> sccs("scc_vtx.bin", "scc_vindex.bin", argv[1]);
	sccs.open();
	

	RawUnit<it> &l0 = levels[0];
	

	ID id = 0;
	for(cnt_t i = 0; i < map_nto.size(); i++ )
	{
		cout << id << endl;
		if(map_nto[i] < id)
		{
			id = map_nto[i + 1];
			break;
		}
		id = map_nto[i];
	}

	cout << "Topo top is " << id << endl;
	map_nto.close();
	levels.close();
	sccs.close();
	return 0;
}
