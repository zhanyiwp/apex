#include <fstream>
#include "string_func.h"
#include "conf_file.h"

namespace Config 
{
	section::section()
	{
	}

	section::section(const std::string& file_path, const std::string& section)
	{
		_file_path = file_path;
		_section = section;
		inner_load();
	}

	section::section(const section& other)
	{
		_file_path = other._file_path;
		_section = other._section;
		_value = other._value;
	}

	section::~section()
	{

	}

	void section::load(const std::string& file_path, const std::string& section)
	{
		_file_path = file_path;
		_section = section;
		_value.clear();
		inner_load();
	}

	std::string section::get_name()
	{
		return _section;
	}

	bool section::inner_load()
	{
		std::ifstream ifs;
		ifs.open(_file_path.c_str(), std::ios::in);
		if (ifs.fail())
			return false;

		std::string app;
		std::string name;
		std::string value;
		std::string line;
		while (getline(ifs, line))
		{
			if (line.empty())
			{
				continue;
			}

			unsigned int i = 0;
			str_trim(line);

			char ch = line[i];
			if ('#' != ch && ';' != ch)
			{
				bool good = true;
				if ('[' == ch)
				{
					std::string::size_type  j = line.find(']', i);
					if (std::string::npos != j)
					{
						app = line.substr(i + 1, j - i - 1);
						str_trim(app);
						if (app.empty())
						{
							good = false;
						}
					}
					else
					{
						good = false;
					}
				}
				if (good && lowstring(app) == lowstring(_section))
				{
					unsigned int j = (unsigned int)line.find('=', i);
					if (j > i)
					{
						name = line.substr(i, j - i);
						str_trim(name);
						if (!name.empty())
						{
							value = line.substr(j + 1);
							str_trim(value);
							_value[name] = value;
						}
					}
				}
			}
		}
		return true;
	}

	std::string get_str(const std::string& file_path, const std::string& section_name, const std::string& field_name)
	{
		section	sec(field_name, section_name);
		std::string str;
		sec.get(field_name, str);
		return str;
	}
}
