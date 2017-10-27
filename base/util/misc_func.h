#ifndef MISC_FUNC_H_
#define MISC_FUNC_H_

#include "base_type.h"	
#include <stdint.h>
#include <stddef.h>

/**
	* get error description of specified errno.
	* on success, the description length was returned. otherwise, 0 is returned.
	* this function is thread-safe.
	*/
int strerr_r(int errnum, char* buf, unsigned int len);

/*
	* set the fd limit for current process.
	* if current fd limit is greater than 'max_fd', nothing is done and return 0.
	* on success, zero is returned. otherwise, -1 is returned.
	* */
int set_file_limit(int max_fd);

/*
	* make current process to be a daemon.
	*/
void daemon_proc();

/*
	* set the core file size limit for current process.
	* if current size limit is greater than 'max_size', nothing is done and return 0.
	* on success, zero is returned. otherwise, -1 is returned.
	*/
int set_core_limit(uint64_t max_size);

/**
	* reopen standard file handler to /dev/null
	* return zero on success. otherwise -1 is returned.
	*/
int reopen_std_file();

/**
	* ignore SIGPIPE
	*/
void ignore_sigpipe();
	
/**
	* dump a mem segment into hex text and print the text to stdout.
	*/
void print_dump(const void* mem, unsigned int len);

/**
	* convert string from one charset to another.
	* return new length of to_str if convert success.
	* return a negative value on error.
	*/
int iconv(char* to_str, size_t to_str_len, const char* to_code, const char* from_str, size_t from_str_len, const char* from_code);

/**
	* create a iconv object to do more convertion
	* Note: the above util_iconv() is easy to use but slow, because it create and release the iconv object each time.
	*/
void* iconv_open(const char* to_code, const char* from_code);
int	iconv_conv(void* cd, char* to_str, size_t to_str_len, const char* from_str, size_t from_str_len);
//void iconv_close(void* cd);


/**
	* some string hash functions
	* see more details: http://www.cse.yorku.ca/~oz/hash.html or google: string hash.
	* to generate a 32bit hash value, util_strhash_sdbm() is recommended.
	* util_strhash_u64() will combine util_strhash_sdbm() and util_strhash_djb2() together to generate a 64bit value.
	*/
unsigned int buff_hash_djb2(const unsigned char *str, int len);
unsigned int buf_hash_sdbm(const unsigned char *str, int len);

/**
	* crc16 check sum
	*/
uint16_t crc16(uint16_t* buf, int nwords);

/**
	* zlib wrappers
	* return the output length on success, return -1 on error.
	*/
int zlib_zip(uint8_t* output, uint32_t output_max_len, uint8_t* input, uint32_t input_len);
int zlib_unzip(uint8_t* output, uint32_t output_max_len, uint8_t* input, uint32_t input_len);

#endif //MISC_FUNC_H_
