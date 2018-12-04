#pragma once

#include <string>

//Proxy Object for parsing raw istream
class Parse
{
public:
	//Overload formated stream
	friend std::istream& operator>>(std::istream& in, Parse &x)
	{
		constexpr char first = 0x1;
		for (char buff, flag = 0; in.get(buff); flag |= first)
		{
			if (!(flag & first) && buff == ' ' || buff == '\n')
			{
				in.ignore(std::numeric_limits<std::streamsize>::max(), '<');
				buff = '<';
			}

			else if (flag & first && buff == '<')
			{
				in.unget();
				break;
			}

			x.m_dat.push_back(std::move(buff));

			if (flag & first && x.m_dat.back() == '>')
				break;
		}

		//Release unused memory
		x.m_dat.shrink_to_fit();
		return in;
	}

	//String data
	std::string m_dat;
};