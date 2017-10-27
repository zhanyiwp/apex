#ifndef LOG_HELPER_H
#define LOG_HELPER_H

#include <stdint.h>
#include <stdarg.h>
#include <string>
#include "duplet_log.h"
#include "mmap_queue.h"

//---------------Log-------------------
int	write_log_nocache(std::string file_path, const uint32_t length_per_file, const uint8_t file_cycle_num, const Gaz::Log::LOG_LEVEL level, const struct timeval tv, const std::string keyword, const std::string& text);
struct mmap_queue* open_log_queue();

#endif
