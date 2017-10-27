#ifndef FILE_FUNC_H_
#define FILE_FUNC_H_

#include "base_type.h"

int64_t get_file_size(const char* file_name);
int64_t get_file_size(int fd);
int		is_path_regular(const char* file_name);
int		is_path_dir(const char* file_name);

/**
* make sure the specified dir exists(if dir not exists, it will try to create the dir)
* non-zero is returned if dir exists, otherwise 0 is returned.
*/
int		dir_ensurance(const char* dir_name);

#endif 
