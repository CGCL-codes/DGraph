#include "mmap_file.h"

Status MMapFile::open(const char* mode)
{
	fp = fopen(dir_name.c_str(), mode);
	
	if(fp == NULL)
	{ 
		cout << "open file " << dir_name << " failed" << endl;
		return ERROR;
	}
	fd = fileno(fp);	
	
	struct stat sb;
	fstat(fd, &sb);	

	flen = sb.st_size;
	if(flen == 0)
	{
		flen = BUFF_SIZE;
		ftruncate(fd, flen);
	}

	fstart = (unsigned char*)mmap(NULL, flen, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	
	if(fstart == MAP_FAILED)	
	{
		cout << "open: mmap file failed" << endl;
		this->close();
		return ERROR;
	}

	is_opened = true;
	return OK;
}
       
Status MMapFile::write(offset_t start, offset_t len, const void* wbuffer)
{
	if(!is_opened)
	{
		cout << "write: file is not opened" << endl;
		return ERROR;
	}

/*	if(start + len > flen)
	{
		offset_t add = (start + len - 1 - flen) / BUFF_SIZE + 1;
		offset_t old_flen = flen;
		flen += add * BUFF_SIZE;
		ftruncate(fd, flen);
		
		//re-mmap file to fix the new length of mmap file        	
		fstart = (unsigned char*)mremap((void*)fstart, old_flen, flen, MREMAP_MAYMOVE);
        	//fstart = (unsigned char*)mmap(NULL, flen, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
		if(fstart == MAP_FAILED)
        	{
                	cout << "write: re-mmap file failed" << endl;
                	this->close();
                	return ERROR;
        	}
	}
*/
	if(expand(start + len) == ERROR)
	{
		cout << "write: expand failed." << endl;
		return ERROR;
	}

	unsigned char *dest = fstart + start; 
	memcpy(dest, wbuffer, len);
	return OK;
}

Status MMapFile::read(offset_t start, offset_t len, void* rbuffer)
{
	if(!is_opened)
	{
	   	cout << "read: file is not opened" << endl;
                return ERROR;
	}

	if(start + len > flen)
	{
		cout << "read: read file out of range" << endl;
		return ERROR;
	}

	unsigned char *src = fstart + start;
	memcpy(rbuffer, src, len);
	return OK;
}

addr_t MMapFile::map_data(offset_t start, offset_t len)
{
        if(!is_opened)
        {
                cout << "map_data: file is not opened" << endl;
                return NULL;
        }

        if(expand(start + len) == ERROR)
        {
                cout << "map_data: expand failed, read file out of range" << endl;
                return NULL;
        }

        return (fstart + start);
}

Status MMapFile::expand(offset_t off) //expand space to [0, off]
{
	if(off < flen)
		return OK;

	Status ret = OK;
	#pragma omp critical
	{
		if(off >= flen)
        	{
                	offset_t add = (off - flen) / BUFF_SIZE + 1;
                	offset_t old_flen = flen;
			flen += add * BUFF_SIZE;
                	ftruncate(fd, flen);

			//mremap is quicker than munmap + mmap
			fstart = (unsigned char*)mremap((void*)fstart, old_flen, flen, MREMAP_MAYMOVE);
			//munmap((void*)fstart, old_flen);
        		//fstart = (unsigned char*)mmap(NULL, flen, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
			if(fstart == MAP_FAILED)
        		{
                		cout << "expand: re-mmap file failed" << endl;
                		this->close();
        	        		ret =  ERROR;
        		}
		}
	}
	return ret;
}

Status MMapFile::close()
{
	munmap(fstart, flen);
	fclose(fp);
	is_opened = false;
	fd = 0;
	fp = NULL;
	flen = 0;
	return OK;
}

Status MMapFile::destroy()
{
	close();
	return remove(dir_name.c_str()) == 0 ? OK : ERROR;
}

/*
 *	OutFile
 */

Status OutFile::write(const char* buff, size_t len)
{
	//Fill the buffer
	if(wp + len > BUFF_SIZE)
	{
		offset_t remain = BUFF_SIZE - wp;
		memcpy(buffer + wp, buff, remain);
		fos.write(buffer, BUFF_SIZE);
		wp = 0;
		len -= remain;
		buff += remain;

		//write big chunks if any	
		if(len > BUFF_SIZE)
		{
			remain = len - (len / BUFF_SIZE) * BUFF_SIZE;
			fos.write(buff, len - remain);
			buff += (len - remain);
			len = remain;
		}
	}
	memcpy(buffer, buff, len);
	wp += len;
	return OK;
}


Status OutFile::flush()
{
	fos.write(buffer, wp);
	wp = 0;
	return OK;
}

Status OutFile::close()
{
	Status s = flush();
	if(s != OK) return s;
	fos.close();
	return OK;
}






