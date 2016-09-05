#ifndef MMAP_FILE
#define MMAP_FILE

#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "type.h"

using namespace std;

const offset_t BUFF_SIZE = 0x4000; //16K

class MMapFile{
protected:
	bool is_opened;
	FILE *fp;
	int fd;
	offset_t flen;
	addr_t fstart;
public:
	string dir, name, dir_name;
	
	MMapFile(string name, string dir ): dir(dir), name(name), dir_name(dir + "/" + name), is_opened(false){};

	Status open(const char* mode = "a+");
	Status write(offset_t start, offset_t len, const void* wbuffer);
	Status read(offset_t start, offset_t len, void* rbuffer);
	addr_t map_data(offset_t start, offset_t len = 1);
	Status close();
	Status destroy();	

	Status expand(offset_t off); //expand file length flen to off

	addr_t &get_file_start(){return fstart;};
	offset_t get_file_len(){return flen;};

};

/*
 * 	OutFile is consulted with PathGraph's class TempFile.
 *	Thank the authors of PathGraph.
 * */

class OutFile 
{
protected:
	int fos;
	offset_t wp;
	char buffer[BUFF_SIZE];
public:
	string dir, name, dir_name;
	
	OutFile(string name, string dir):
		dir(dir), name(name), dir_name(dir + "/" + name),
		wp(0){
			fos = open((dir + "/" + name).c_str(), 
					O_WRONLY | O_CREAT | O_TRUNC, 
					S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		};

	Status write(const char* buff, size_t len);
	Status flush();
	Status close();

	template<class T>
	Status write_unit(T unit)
	{
		for(int i = 0; i < sizeof(T); i++)
		{
			if(wp >= BUFF_SIZE)
			{	
				::write(fos, buffer, wp);
				wp = 0;
			}
			char c = static_cast<unsigned char> (unit & 0xff);
			buffer[wp++] = c;
			unit >>= 8;
		}
		return OK;
	}

};

#endif

