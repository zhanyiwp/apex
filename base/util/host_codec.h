#ifndef HOST_CODEC_H
#define HOST_CODEC_H

#include <stdint.h>
#include <string.h>

#include <string>

class host_codec
{
public:
	/// constructor
	inline host_codec() :m_good(true), m_data(NULL), m_length(0), m_read(0), m_write(0) {}
	template <typename T>
	inline host_codec(const uint32_t length, T *data) : m_good(true), m_data((uint8_t*)(void*)data), m_length(length), m_read(0), m_write(0) {}
	virtual ~host_codec() {}

	/// set data segment
	inline void set(uint32_t length, uint8_t *data){ m_data = data;	m_length = length; m_good = true; m_read = 0; m_write = 0; }

	/// get the data pointer
	inline uint8_t* data() const { return m_data; }

	/// get the length of data buffer
	inline uint32_t length() const { return m_length; }

	/// get space to write
	inline uint32_t space() const { return m_length - m_write; }

	/// get bytes available to read
	inline uint32_t available() const { return m_length - m_read; }

	/// get current write position
	inline uint32_t wpos() { return m_write; }

	/// set current write position
	inline void wpos(uint32_t p) { m_write = p; }

	/// get current read position
	inline uint32_t rpos() { return m_read; }

	/// set current read position
	inline void rpos(uint32_t p) { m_read = p; }

	/// reset both read and write pointers
	inline void reset() { m_read = m_write = 0; m_good = true; }

	/// whether the stream works well
	inline bool good() const { return m_good; }
	inline operator bool() const { return m_good; }

	/// move write pointer for some steps
	host_codec& write(uint32_t step);
	/// move read pointer for some steps
	host_codec& read(uint32_t step);

	/// write something
	host_codec& write(uint32_t length, const void* data);

	template <typename T>  
	host_codec& operator << (const T& t)
	{
		if (m_good)
		{
			if (m_write + sizeof(T) > m_length)
			{
				m_good = false;
			}
			else
			{
				*reinterpret_cast<T*>(m_data + m_write) = t;
				m_write += sizeof(T);
			}
		}
		return *this;
	}
	
	// read something
	host_codec& read(uint32_t length, void* data);

	template <typename T>
	host_codec& operator >> (T& t)
	{
		if (m_good)
		{
			if (m_read + sizeof(T) > m_length)
			{
				m_good = false;
			}
			else
			{
				t = *reinterpret_cast<const T*>(m_data + m_read);
				m_read += sizeof(T);
			}
		}
		return *this;
	}

private:
	/*volatile */bool	m_good;
	/*volatile */uint8_t*	m_data;
	/*volatile */uint32_t	m_length;
	/*volatile */uint32_t	m_read;
	/*volatile */uint32_t	m_write;
};

bool AppendU8Str(host_codec& os, const char* pszText, uint8_t u8Len = (uint8_t)-1);
bool AppendU16Str(host_codec& os, const char* pszText, uint16_t u16Len = (uint16_t)-1);
bool ReadU8Str(host_codec& is, std::string& str);
bool ReadU16Str(host_codec& is, std::string& str);

#endif