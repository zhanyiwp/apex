#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <mysql.h>
#include <map>
#include <string>

#pragma once

namespace asyncmysql
{
    typedef unsigned char u8;
    typedef signed char s8;
    typedef unsigned short u16;
    typedef signed short s16;
    typedef unsigned int u32;
    typedef signed int s32;
    typedef unsigned long long u64;
    typedef signed long long s64;
    typedef float f32;
    typedef double f64;
    typedef long double f128;

    typedef std::map<std::string, u32> map_index;
    typedef std::string blob;

    class result_set
    {

    public:
        result_set(MYSQL_RES *result_set);
        ~result_set();

        blob get_blob(const char *name) const;
        const char *get_string(const char *name) const;
        bool get_boolean(const char *name) const;
        u8 get_unsigned8(const char *name) const;
        s8 get_signed8(const char *name) const;
        u16 get_unsigned16(const char *name) const;
        s16 get_signed16(const char *name) const;
        u32 get_unsigned32(const char *name) const;
        s32 get_signed32(const char *name) const;
        u64 get_unsigned64(const char *name) const;
        s64 get_signed64(const char *name) const;
        f32 get_float(const char *name) const;
        f64 get_double(const char *name) const;
        bool is_null(const char *name) const;

        blob get_blob(u32 index) const;
        const char *get_string(u32 index) const;
        bool get_boolean(u32 index) const;
        u8 get_unsigned8(u32 index) const;
        s8 get_signed8(u32 index) const;
        u16 get_unsigned16(u32 index) const;
        s16 get_signed16(u32 index) const;
        u32 get_unsigned32(u32 index) const;
        s32 get_signed32(u32 index) const;
        u64 get_unsigned64(u32 index) const;
        s64 get_signed64(u32 index) const;
        f32 get_float(u32 index) const;
        f64 get_double(u32 index) const;
        bool is_null(u32 index) const;

        u64 column_count() const;
        u32 column_index(const char *name) const;
        const char *column_name(u32 index);

        u64 row_index() const;
        u64 row_count() const;

        bool next();

        bool set_row_index(u64 index);

    private:
        u32 m_field_count;
        u64 m_row_count;
        long unsigned int *m_lengths;
        MYSQL_RES *m_result_set;
        MYSQL_FIELD *m_fields;
        MYSQL_ROW m_row;
        map_index m_indexes;
    };
}