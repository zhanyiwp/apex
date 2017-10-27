#include <stdlib.h>
#include "async_mresult.h"
using namespace asyncmysql;

result_set::result_set(MYSQL_RES *result_set)
:m_field_count(0)
,m_row_count(0)
,m_lengths(NULL)
,m_result_set(result_set)
,m_fields(NULL)
{
	if (m_result_set)
	{
		m_field_count = mysql_num_fields(m_result_set);
        m_row_count = (u64)mysql_num_rows(m_result_set);
		m_fields = mysql_fetch_fields(m_result_set);
		for (u32 i = 0; i < m_field_count; ++i)
        {
			m_indexes[m_fields[i].name] = i;
        }
	}
}

result_set::~result_set()
{
	if (m_result_set)
    {   
		mysql_free_result(m_result_set);
        m_result_set = NULL;
    }
}

blob result_set::get_blob(const char* name) const
{
	return get_blob(column_index(name));
}

const char* result_set::get_string(const char* name) const
{
	return get_string(column_index(name));
}

bool result_set::get_boolean(const char* name) const
{
	return get_boolean(column_index(name));
}

u8 result_set::get_unsigned8(const char* name) const
{
	return get_unsigned8(column_index(name));
}

s8 result_set::get_signed8(const char* name) const
{
	return get_signed8(column_index(name));
}

u16 result_set::get_unsigned16(const char* name) const
{
	return get_unsigned16(column_index(name));
}

s16 result_set::get_signed16(const char* name) const
{
	return get_signed16(column_index(name));
}

u32 result_set::get_unsigned32(const char* name) const
{
	return get_unsigned32(column_index(name));
}

s32 result_set::get_signed32(const char* name) const
{
	return get_signed32(column_index(name));
}

s64 result_set::get_signed64(const char* name) const
{
	return get_signed64(column_index(name));
}

u64 result_set::get_unsigned64(const char* name) const
{
	return get_unsigned64(column_index(name));
}

f32 result_set::get_float(const char* name) const
{
	return get_float(column_index(name));
}

f64 result_set::get_double(const char* name) const
{
	return get_double(column_index(name));
}

bool result_set::is_null(const char* name) const
{
	return is_null(column_index(name));
}

blob result_set::get_blob(u32 index) const
{
    assert(index <= m_field_count);
    blob s;
	size_t len = m_lengths[index];
	if (!len || !m_row[index])
		return s;
    s.assign(m_row[index], len);
	return s;
}

const char* result_set::get_string(u32 index) const
{
    assert(index <= m_field_count);
	return m_row[index];
}

bool result_set::get_boolean(u32 index) const
{
    assert(index <= m_field_count);
	return m_row[index] == NULL ? false : (bool)(atoi(m_row[index]));
}

u8 result_set::get_unsigned8(u32 index) const
{
    assert(index <= m_field_count);
	return m_row[index] == NULL ? 0 :(u8)(atoi(m_row[index]));
}

s8 result_set::get_signed8(u32 index) const
{
    assert(index <= m_field_count);
	return m_row[index] == NULL ? 0 :(s8)(atoi(m_row[index]));
}

u16 result_set::get_unsigned16(u32 index) const
{
    assert(index <= m_field_count);
	return m_row[index] == NULL ? 0 : (u16)(atoi(m_row[index]));
}

s16 result_set::get_signed16(u32 index) const
{
    assert(index <= m_field_count);
	return  m_row[index] == NULL ? 0 : (s16)(atoi(m_row[index]));;
}

u32 result_set::get_unsigned32(u32 index) const
{
    assert(index <= m_field_count);
	return m_row[index] == NULL ? 0 : (u32)(atoi(m_row[index]));
}

s32 result_set::get_signed32(u32 index) const
{
    assert(index <= m_field_count);
	return m_row[index] == NULL ? 0 : (s32)(atoi(m_row[index]));
}

u64 result_set::get_unsigned64(u32 index) const
{
    assert(index <= m_field_count);
	return m_row[index] == NULL ? 0 : (u64)(atoll(m_row[index]));
}

s64 result_set::get_signed64(u32 index) const
{
    assert(index <= m_field_count);
	return m_row[index] == NULL ? 0 : (s64)(atoll(m_row[index]));
}

f32 result_set::get_float(u32 index) const
{
    assert(index <= m_field_count);
	return m_row[index] == NULL ? 0.0 : (f32)(atof(m_row[index]));
}

f64 result_set::get_double(u32 index) const
{
    assert(index <= m_field_count);
	return m_row[index] == NULL ? 0.0 :(f64)(atof(m_row[index]));
}

bool result_set::is_null(u32 index) const
{
    assert(index <= m_field_count);
	return !m_row[index];
}

u64 result_set::column_count() const
{
	return m_field_count;
}

const char* result_set::column_name(u32 index)
{
    assert(index <= m_field_count);
	return m_fields[index].name;
}

u32 result_set::column_index(const char* name) const
{
	const map_index::const_iterator i = m_indexes.find(name);
	if (i == m_indexes.end())
		return 0xffffffff;
	return i->second;
}

bool result_set::set_row_index(u64 index)
{
	mysql_data_seek(m_result_set, index);
	return next();
}

bool result_set::next()
{
	m_row = mysql_fetch_row(m_result_set);
	m_lengths = mysql_fetch_lengths(m_result_set);
	return m_row != NULL;
}

u64 result_set::row_index() const
{
	return (u64)mysql_row_tell(m_result_set);
}

u64 result_set::row_count() const
{
	return m_row_count;
}

