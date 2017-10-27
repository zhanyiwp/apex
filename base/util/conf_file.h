#ifndef CONF_FILE_H_
#define CONF_FILE_H_

#define __STDC_LIMIT_MACROS

#include <stdint.h>
#include <map>
#include <sstream>
#include <string>

namespace Config
{
	class section
	{
	public:
		section();
		section(const std::string& file_path, const std::string& section);
		section(const section& other);
		virtual ~section();

		void		load(const std::string& file_path, const std::string& section);
		std::string	get_name();

		template<class T>
		T& get(const std::string& name, T& value, const std::string& def = "")
		{
			std::string val;

			KEY_VALUE::iterator subit = _value.find(name);
			if (subit != _value.end())
			{
				val = subit->second;
			}
			else
			{
				val = def;
			}
			std::stringstream sstream(val);
			sstream >> value;

			return value;
		}
	private:
		bool inner_load();

		std::string		_file_path;
		std::string		_section;

		typedef std::map< std::string, std::string > KEY_VALUE;
		KEY_VALUE _value;
	};

	std::string get_str(const std::string& file_path, const std::string& section_name, const std::string& field_name);
}

#endif //CONF_FILE_H_