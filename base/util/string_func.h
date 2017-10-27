#ifndef STRING_FUNC_H
#define STRING_FUNC_H

#include <stdint.h>
#include <unistd.h>

#include <string>

using namespace std;

size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);

string lowstring(const string str);

void   str_trim(std::string& str);

#endif