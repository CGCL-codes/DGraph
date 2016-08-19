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
    if (argc != 4)
    {
        cout << "Usage ./analysis <data dir> <to dir> <name>" << endl;
        return 0;
    }
	string dir(argv[1]), to_dir(argv[2]), name(argv[3]), name_dir(to_dir + string("/") + name);
	
	RawFile<ID> scc_group("scc_group.bin", dir);
	scc_group.open();

	VtxsFile<ID> scc_vtxs("scc_vtx.bin", "scc_vindex.bin", dir);
	scc_vtxs.open();

	FILE *fp;
	fp = fopen(name_dir.c_str(), "w+");



	fprintf(fp, "digraph G{\n" );
    string agm(
        string("graph[overlap = scale, rankdir = LR];\n") + 
        string("node[shape = point, color = red];\n")
        + string("edge[arrowsize = 0.1, penwidth = 0.1, color = gray];\n")
        );  
    fprintf(fp, "%s", agm.c_str());

	stringstream ss;	

	for(ID i = 0; i < scc_vtxs.size(); i++)
	{
		ss << i;
		addr_t base = scc_vtxs.get_vtx_base_addr(i);
		if(base == NULL)
			ss << ";\n";
		else
		{
			ss << "->{";
			ID *outbor = VtxUnitIdData::bor(base);
			bor_cnt_t size = VtxUnitIdData::bor_cnt(base);
			for(ID j = 0; j < size; j++)
				ss << outbor[j] << ";";
			ss << "};\n";
		}
		fprintf(fp, "%s", ss.str().c_str());
		ss.str("");
	}


	fprintf(fp, "}\n");
	fclose(fp);
	scc_group.close();
	scc_vtxs.close();
	return 0;
}
