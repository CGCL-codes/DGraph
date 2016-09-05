#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <fstream>

using namespace std;

typedef long long ll;
typedef long long ID;

const ll maxlen = 10000000;

int main(int argc, const char *argv[])
{
	if(argc != 3)
	{
		cout << "Usage: convert <adjlist file name> <new edgelist file name>" << endl;
		return 0;
	}

	FILE *f = fopen(argv[1], "r");
	freopen(argv[2], "w", stdout);
//	ofstream outfile;
//	outfile.open(argv[2], ios::out);
	
	fprintf(stderr, "INFO: Start read dataset %s\n", argv[1]);
	ID from, N = 0;
    char *s = (char*)malloc(maxlen);
    char delims[] = " \t\n";

    ll brcnt = 0;
    while (fgets(s, maxlen, f) != NULL)
    {   
        if (s[0] == '#' || s[0] == '%' || s[0] == '\n')
        {
			//outfile << s << endl;
			printf("%s\n", s);
			continue; //Comment
		}

        char *t = strtok(s, delims);
        from = atoi(t);
        N = max(N, from);

        ID to; 
        while ((t = strtok(NULL, delims)) != NULL)
        {   
            to = atoi(t);
            N = max(N, to);
			
			printf("%ld %ld\n", from, to);
			//outfile << from << " " << to << endl;
            brcnt++;    
        } 
        if (from % 100000 == 0)
			fprintf(stderr,"INFO: %ld\n", from);
	}
	
	fprintf(stderr, "INFO: edge num: %ld \n", brcnt);
//	outfile.close();
	fclose(f);
	return 0;
}
