/*
 *	Generate scc data: scc id, vtxs in scc, scc in, scc out
 */
#include "gfile.h"
#include <string>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <algorithm>


int main(int argc, char *argv[])
{
    	if (argc != 2)
    	{	
        	cout << "Usage ./get_topo_top <data dir>" << endl;
        	return 0;
    	}
/*
	VtxIndexFile<ID> vidin("map_pg_vtxs_in.bin", argv[1]), vidout("map_pg_vtxs_out.bin", argv[1]);
	vidin.open();
	vidout.open();
	
	for(int i = 0; i < vidin.size(); i++)
	{
		cout << i << "-" << vidin[i] << "-" << vidout[i] << endl;
	}

	vidin.close();
	vidout.close();
*/
	
	VtxsFile<ID> vtxs_in("order_vtxs_in.bin", "order_vindex_in.bin", argv[1]),
				 vtxs_out("order_vtxs_out.bin", "order_vindex_out.bin", argv[1]);
	vtxs_in.open();
	vtxs_out.open();

	for(int i = 0; i < max(vtxs_in.size(), vtxs_out.size()); i++)
	{
		ID a, b;
		VtxUnit<ID> *vu = vtxs_in[i];
		a =  *(vu->id_p);
		delete vu;
		vu = vtxs_out[i];
		b =  *(vu->id_p);
		delete vu;
		if(a != b)
			cout << i << "-" << a << "-" << b << endl;
	}

	vtxs_in.close();
	vtxs_out.close();


	return 0;
}
