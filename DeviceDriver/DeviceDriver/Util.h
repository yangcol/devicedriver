#pragma once
#include <stdlib.h>
#include <string>

//!Function List

//!Convert String in Dec mode into one byte.
int StringToHEX_OneByte(std::string strContent, unsigned char &number);
int StringToHEX(std::string strContent, int &number);
int StringToNumber(std::string strContent);
int SeparateParameters(std::string strLine, std::vector<std::string> &vectorParameters);
//default to be Dec mode
int StringToNumber(std::string strContent)
{
	int num = 0;
	for (int i=0; i != strContent.size(); ++i)
	{
		if (strContent.at(i) >= '0' && strContent.at(i) <= '9')
		{
			num = 10*num + strContent.at(i) - '0';
		} 
		else 
		{
			return -1;
		}
	}
	return num;
}

int StringToHEX(std::string strContent, int &number)
{
	int num = 0;
	for (size_t i=0; i != strContent.size(); ++i)
	{
		if (strContent.at(i) >= '0' && strContent.at(i) <= '9')
		{
			num = 16*num + strContent.at(i) - '0';
		} 
		else if (strContent.at(i) >= 'a' && strContent.at(i) <= 'f')
		{
			num = 16*num + strContent.at(i) - 'a' + 10;
		}
		else if (strContent.at(i) >= 'A' && strContent.at(i) <= 'F')
		{
			num = 16*num + strContent.at(i) - 'A' + 10;
		}
		else 
		{
			return -1;
		}
	}

	number = num;
	return 0;
}

int StringToHEX_OneByte(std::string strContent, unsigned char &number)
{
	//if starts with 0x, ignore it
	if (strContent.empty())
	{
		return -1;
	}

	//Trim 0x
	if (strContent.size() > 2)
	{
		//judge if starts from 0x
		std::string::size_type pos1 =	strContent.find_first_of("0x", 0);
		std::string::size_type pos2 = strContent.find_first_of("0X", 0);

		if (-1 != pos1 || -1 != pos2)
		{
			std::string::size_type pos = pos1 > pos2 ? pos1: pos2;
			strContent.assign(strContent.begin()+pos + 2, strContent.end());
		}
	}

	if (strContent.size() > 2)
	{
		return -1;
	}

	int tempNumber = 0;
	if (0 != StringToHEX(strContent, tempNumber))
	{
		return -1;
	}
	number = tempNumber;
	return 0;
	//return ;
}

static int SeparateParameters(std::string strLine, std::vector<std::string> &vectorParameters)
{
	vectorParameters.clear();

	std::string strTemp;
	for (std::string::size_type i=0; i != strLine.size(); ++i)
	{
		if (' ' != strLine.at(i) && '\t' != strLine.at(i))
		{
			strTemp.push_back(strLine.at(i));
			/*inputed_parameters.push_back(strTemp);*/
			continue;
		}

		if (!strTemp.empty())
		{
			vectorParameters.push_back(strTemp);
			strTemp.clear();
		}		
	}

	if (!strTemp.empty())
	{
		vectorParameters.push_back(strTemp);
	}
	//push the last string
	return 0;
}

/*
template< typename Ptr >
void check_null_ptr( const Ptr& ptr, const std::string& parameter_name ) const {
	if ( 0 == ptr ) {
		const std::string what_happened =
			"Zero pointer (function/object/method) detected for parameter '" + parameter_name + "'!";
		throw std::invalid_argument( what_happened );
	} else {}
*/