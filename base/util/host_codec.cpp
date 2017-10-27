#include "host_codec.h"

host_codec& host_codec::write(uint32_t step)
{
	if (m_good)
	{
		m_write += step;
		if (m_write > m_length)
		{
			m_write = m_length - 1;
			m_good = false;
		}
	}
	return *this;
}

host_codec& host_codec::read(uint32_t step)
{
	if (m_good)
	{
		m_read += step;
		if (m_read > m_length)
		{
			m_read = m_length - 1;
			m_good = false;
		}
	}
	return *this;
}

host_codec& host_codec::write(uint32_t length, const void* data)
{
	if (m_good)
	{
		if (m_write + length > m_length)
		{
			m_good = false;
		}
		else
		{
			memcpy(reinterpret_cast<void*>(m_data + m_write), data, length);
			m_write += length;
		}
	}
	return *this;
}

host_codec& host_codec::read(uint32_t length, void* data)
{
	if (m_good)
	{
		if (m_read + length > m_length)
		{
			m_good = false;
		}
		else
		{
			memcpy(data, reinterpret_cast<const void*>(m_data + m_read), length);
			m_read += length;
		}
	}
	return *this;
}

/////////////////////
bool AppendU8Str(host_codec& os, const char* pszText, uint8_t u8Len/* = (uint8_t)-1*/)
{
	uint32_t	u32Len = u8Len;
	if (u8Len == (uint8_t)-1)
	{
		u32Len = strlen(pszText);
		if (u32Len > uint8_t(-1))
		{
			return false;
		}
	}
	os << static_cast<uint8_t>(u32Len);
	os.write(u32Len, pszText);
	if (!os)
	{
		return false;
	}

	return true;
}

bool AppendU16Str(host_codec& os, const char* pszText, uint16_t u16Len/* = (uint16_t)-1*/)
{
	uint32_t	u32Len = u16Len;
	if (u16Len == (uint16_t)-1)
	{
		u32Len = strlen(pszText);
		if (u32Len > uint16_t(-1))
		{
			return false;
		}
	}

	os << static_cast<uint16_t>(u32Len);
	os.write(u32Len, pszText);
	if (!os)
	{
		return false;
	}

	return true;
}

bool ReadU8Str(host_codec& is, std::string& str)
{
	uint8_t	u8Len = 0;
	is >> u8Len;
	if (!is)
	{
		return false;
	}

	str = std::string((char*)&is.data()[is.rpos()], u8Len);
	is.read(u8Len);
	if (!is)
	{
		return false;
	}

	return true;
}

bool ReadU16Str(host_codec& is, std::string& str)
{
	uint16_t	u16Len = 0;
	is >> u16Len;
	if (!is)
	{
		return false;
	}

	str = std::string((char*)&is.data()[is.rpos()], u16Len);
	is.read(u16Len);
	if (!is)
	{
		return false;
	}

	return true;
}