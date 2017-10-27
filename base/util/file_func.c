#include <sys/stat.h>
#include <string.h>
#include "file_func.h"

int64_t get_file_size(const char* file_name)
{
	struct stat buf;
	if (stat(file_name, &buf) < 0)
		return -1;
	return (int64_t)buf.st_size;
}

int64_t get_file_size(int fd)
{
	struct stat buf;
	if (fstat(fd, &buf) < 0)
		return -1;
	return (int64_t)buf.st_size;
}

int is_path_regular(const char* file_name)
{
	struct stat buf;
	if (stat(file_name, &buf) < 0)
		return 0;
	return (int)(S_ISREG(buf.st_mode));
}

int is_path_dir(const char* file_name)
{
	struct stat buf;
	if (stat(file_name, &buf) < 0)
		return 0;
	return (int)(S_ISDIR(buf.st_mode));
}

static int dir_ensurance_inner(const char* dir_name, unsigned int total_length, unsigned int offset)
{
	char dir_tmp[1024];
	struct stat buf;

	char* pos = strchr((char*)dir_name + offset, '/');
	if (NULL == pos)
		offset = total_length;
	else
		offset += pos - ((char*)dir_name + offset) + 1;
	memcpy(dir_tmp, dir_name, offset);
	dir_tmp[offset] = '\0';
	if (stat(dir_tmp, &buf) < 0)
	{
		if (mkdir(dir_tmp, 0755) < 0)
			return 0;
	}
	else
	{
		if (!S_ISDIR(buf.st_mode))
			return 0;
	}

	//do the next sub dir check
	if (offset >= total_length)
		return 1;
	else
		return dir_ensurance_inner(dir_name, total_length, offset);
}

int dir_ensurance(const char* dir_name)
{
	return dir_ensurance_inner(dir_name, strlen(dir_name), 0);
}