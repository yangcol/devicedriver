#include "FTDIDeviceDriver.h"
#include "TCDDefine.h"
#include <string>
#ifndef BATCH_TEST
#define BATCH_TEST
#include <iostream>
#include <fstream>
#endif

using namespace::TCDNameSpace::TCDConfiguration;
namespace {
	typedef int (*fun)();
	typedef std::map<std::string, fun>::iterator iterFun;
	std::map<std::string, fun> static_function_MAP;
	const std::string const_help_string = "Input [help] for more information\n";
	const std::string const_not_found_string = " not recognized";
	FTDI_DeviceDriver* static_pDeviceDriver = NULL;
	std::vector<std::string> static_vector_command;
	bool static_bInitializedFlag = false;
	static unsigned char readByte = 0;
	//static_vector_command include function name, so if with no input param, size of command is 1.
	enum 
	{
		INPUT_PARAM_0 = 1,
		INPUT_PARAM_1,
		INPUT_PARAM_2,
		INPUT_PARAM_3,
		INPUT_PARAM_4,
		INPUT_PARAM_5,
		INPUT_PARAM_6
	};
};

static int call_help();
static int call_list();
static int call_open();
static int call_close();
static int call_load();
static int call_save();
static int call_send();
static int call_receive();
static int call_setaddr();
static int call_configi2c();

static int SeparateParameters(std::string strLine, std::vector<std::string> &vectorParameters);
static int Print_Function_Usage_Error(std::string strCommand);
static int Print_Function_Not_Supported(std::string strCommand);
static int Init_Call_DeviceDriver();

static int StringToHEX_OneByte(std::string strContent, unsigned char &number);

//Float type is not supported
static int StringToNumber(std::string strContent);

int CallDeviceDriver(std::string strCommand)
{
	if (false == static_bInitializedFlag)
	{
		Init_Call_DeviceDriver();
		//return -1;
	}

	if (false == static_bInitializedFlag)
	{
		printf("Init Fail!\n");
	}

	if (strCommand.empty())
	{
		return 0;
	}

	iterFun iterFunctionMap = static_function_MAP.begin();
	SeparateParameters(strCommand, static_vector_command);

	if (static_vector_command.empty())
	{
		//blank vector
		return -1;
	}

	iterFunctionMap = static_function_MAP.find(static_vector_command.at(0));

	if (static_function_MAP.end() == iterFunctionMap)
	{
		//Function with name is not found in function list
		printf("[%s] %s, %s", static_vector_command.at(0).c_str(), const_not_found_string.c_str(), const_help_string.c_str());
		return -1;
	}

	//Function exists
	int result = iterFunctionMap->second();

	return result;
}

static int RegFunc(std::string strfuncName, int (*func)())
{
	iterFun iterFunctionMap = static_function_MAP.begin();
	if (static_function_MAP.end() != (iterFunctionMap = static_function_MAP.find(strfuncName)))
	{
		//A function with the same name is registered
		return -1;
	}

	static_function_MAP.insert(std::pair<std::string, fun>(strfuncName, func));
		//std::pair<int, TuningChannelDriver*>(i, p
	return 0;
}

static int Init_Call_DeviceDriver()
{
	static_function_MAP.clear();

	//Reg all functions
	RegFunc("help", call_help);
	RegFunc("list", call_list);
	RegFunc("open", call_open);
	RegFunc("close", call_close);
	RegFunc("load", call_load);
	RegFunc("save", call_save);
	RegFunc("send", call_send);
	RegFunc("receive", call_receive);
	RegFunc("configi2c", call_configi2c);
	//RegFunc("addr", call_setaddr);

	static_pDeviceDriver = FTDI_DeviceDriver::CreateInstance();
	if (NULL == static_pDeviceDriver)
	{
		return -1;
	}
	static_bInitializedFlag = true;
	return 0;
}

static int call_help()
{
	switch (static_vector_command.size())
	{
	//Others
	//help + 0 params
	case INPUT_PARAM_0:
		{
			//list all functions
			iterFun iterFunctionMap = static_function_MAP.begin();
			printf("Available functions are listed below:\n");
			for (; iterFunctionMap != static_function_MAP.end(); ++iterFunctionMap)
			{
				printf("\t%s\n", iterFunctionMap->first.c_str());
			}
		}
		break;	
	//help + 1 params
	case INPUT_PARAM_1:
		{
			iterFun iterFunctionMap = static_function_MAP.find(static_vector_command.at(1));
			if (static_function_MAP.end() == iterFunctionMap)
			{
				Print_Function_Not_Supported(static_vector_command.at(0));
			}
		}
		break;
	default:
		Print_Function_Usage_Error(static_vector_command.at(0));
		return -1;
	}

	return 0;
}

static int call_list()
{
	int result  = -1;
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_0:
		{
			int devNumber = 0;
			result = static_pDeviceDriver->ListAllDevices(&devNumber);
			printf("Total dev numbers are: %d.\n", devNumber);
		}
		break;
	default:
		Print_Function_Usage_Error(static_vector_command.at(0));
		return -1;
	}
	
	return result;
}

static int call_open()
{
	//static_pDeviceDriver->Open();
	int result  = -1;
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_1:
		{
			std::vector<int> index_opened;
			static_pDeviceDriver->GetOpened(index_opened);

			if (0 != index_opened.size())
			{
				printf("Another channel already opened!\n");
			}
			result = static_pDeviceDriver->Open(StringToNumber(static_vector_command.at(1)));
		}
		break;
	default:
		Print_Function_Usage_Error(static_vector_command.at(0));
		return -1;
	}
	
	if (0 != result)
	{
		printf("Open Fail!\n");
	}
	
	printf("Success!\n");
	return result;
}

static int call_close()
{
	int result  = -1;
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_0:
		{
			std::vector<int> index_opened;
			static_pDeviceDriver->GetOpened(index_opened);

			if (0 != index_opened.size())
			{
				result = static_pDeviceDriver->Close(index_opened.at(0));
			}
		}
		break;
	//case INPUT_PARAM_1:

	//	
	//	break;
	default:
		Print_Function_Usage_Error(static_vector_command.at(0));
		return -1;
	}

	return result;
}

static int call_load()
{
	int result  = -1;
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_1:
		{
			std::vector<int> vector_int;
			static_pDeviceDriver->GetOpened(vector_int);
			if (0 == vector_int.size())
			{
				printf("No device is opened!\n");
				break;
			}
			result = static_pDeviceDriver->LoadConfig(vector_int.at(0), static_vector_command.at(1));
		}
		break;
	case INPUT_PARAM_2:
		result = static_pDeviceDriver->LoadConfig(StringToNumber(static_vector_command.at(1)), static_vector_command.at(2));
		break;
	default:
		Print_Function_Usage_Error(static_vector_command.at(0));
		return -1;
	}

	return result;
}

static int call_save()
{
	int result  = -1;
	switch(static_vector_command.size())
	{
		case INPUT_PARAM_0:
			{
				std::vector<int> vector_int;
				static_pDeviceDriver->GetOpened(vector_int);
				if (0 == vector_int.size())
				{
					printf("No device is opened!\n");
					break;
				}
				//static_pDeviceDriver->Get
				//static_pDeviceDriver->GetOpened()
				result = static_pDeviceDriver->SaveDefaultConfig(vector_int.at(0));
			}
			break;
	case INPUT_PARAM_1:
		{
			std::vector<int> vector_int;
			static_pDeviceDriver->GetOpened(vector_int);
			if (0 == vector_int.size())
			{
				printf("No device is opened!\n");
				break;
			}
			result = static_pDeviceDriver->SaveConfig(vector_int.at(0), static_vector_command.at(1));
		}
		break;
	default:
		Print_Function_Usage_Error(static_vector_command.at(0));
		return -1;
	}

	return result;
}

//Command send act as
//send [addr] [para1] [para2] ...
//separated by space
static int call_send()
{
	int result  = -1;
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_0:
	case INPUT_PARAM_1:
		Print_Function_Not_Supported(static_vector_command.at(0));
		break;
	default:
		{
			//the first param is address, the second is content.
			std::vector<int> vector_int;
			static_pDeviceDriver->GetOpened(vector_int);
			if (0 == vector_int.size())
			{
				printf("No device is opened!\n");
				break;
			}

			const int paramOffSet = 2;	
			const int commandOffset = 1;
			//for (size_t i= commandOffset; i != static_vector_command.size(); ++i)
			//{
			//	if (static_vector_command.at(i).size())
			//	{
			//	}
			//}
			unsigned char addr = 0;
			if (0 != StringToHEX_OneByte(static_vector_command.at(1), addr))
			{
				printf("Input params error!\n");
				return -1;
			}

			unsigned char writeByte = 0;
			size_t sizeWritten;
			static_pDeviceDriver->SetAddress(vector_int.at(0), addr);
			// in command vector, bytes starts from index 2. 0 is command, 1 is address.
			unsigned char *buffer = new unsigned char[static_vector_command.size() - paramOffSet];
			for (size_t i=0; i != static_vector_command.size() - paramOffSet; ++i)
			{
				if (0 != StringToHEX_OneByte(static_vector_command.at(i + paramOffSet), buffer[i]))
				{
					printf("Input params error!\n");
					return -1;
				}
			}

			int result = -1;
			result = static_pDeviceDriver->Transfer(vector_int.at(0), buffer, static_vector_command.size() - paramOffSet, &sizeWritten);
			delete buffer;
			if (0 != result)
			{
				printf("Transfer Failed!");
			}
//#if defined _DEBUG
//			printf("Success\n");
//#endif
		}
		break;
	//default:
	//	Print_Function_Usage_Error(static_vector_command.at(0));
	//	return -1;
	}

	return 0;
}

static int call_receive()
{
	std::vector<int> vector_int;
	static_pDeviceDriver->GetOpened(vector_int);
	if (0 == vector_int.size())
	{
		printf("No device is opened!\n");
		return -1;
	}

	switch(static_vector_command.size())
	{
	//case INPUT_PARAM_0:
	//
	//	Print_Function_Usage_Error(static_vector_command.at(0));
	//	return -1;
		/*result = static_pDeviceDriver->Open(StringToNumber(static_vector_command.at(1)));*/
	case INPUT_PARAM_1:
		{
			unsigned char addr = 0;
			if (0 != StringToHEX_OneByte(static_vector_command.at(1), addr))
			{
				printf("Input params error!\n");
				return -1;
			}
			static_pDeviceDriver->SetAddress(vector_int.at(0), addr);
		}
	default:
		{	
			unsigned char recBuffer[1];
			size_t sizeReceived = 0;

			static_pDeviceDriver->Receive(vector_int.at(0), recBuffer, sizeof(recBuffer), &sizeReceived);

			int number = *recBuffer;
			printf("Value = %X\n", number);
		}
	}

	return 0;
}

static int call_setaddr()
{
	int result = 0;
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_1:
		{
			std::vector<int> vector_int;
			static_pDeviceDriver->GetOpened(vector_int);
			if (0 == vector_int.size())
			{
				printf("No device is opened!\n");
				return -1;
			}
			result = static_pDeviceDriver->SetAddress(vector_int.at(0), StringToNumber( static_vector_command.at(1)));
		}
		
		break;
	default:
		Print_Function_Usage_Error(static_vector_command.at(0));
		return -1;
	}
	return result;
}

//Config i2c + clockrate
static int call_configi2c()
{
	int result = 0;
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_1:
		{
			std::vector<int> vector_int;
			static_pDeviceDriver->GetOpened(vector_int);
			if (0 == vector_int.size())
			{
				printf("No device is opened!\n");
				return -1;
			}
			int number = StringToNumber(static_vector_command.at(1));
			if (TCD_I2C_CLOCK_STANDARD_MODE != number && TCD_I2C_CLOCK_STANDARD_MODE_3P != number\
				&& TCD_I2C_CLOCK_FAST_MODE != number && TCD_I2C_CLOCK_FAST_MODE_PLUS != number \
				&& TCD_I2C_CLOCK_HIGH_SPEED_MODE != number)
			{
				printf ("I2C Speed Error!\n");
			}
			

		}
		break;
	default:
		Print_Function_Usage_Error(static_vector_command.at(0));
		return -1;
	}
	return 0;
}

static int SeparateParameters(std::string strLine, std::vector<std::string> &vectorParameters)
{
	vectorParameters.clear();
	std::string::size_type pos = 0;

	while (-1 != (pos = strLine.find_first_of(' ')) )
	{
		if (0 == pos)
		{
			strLine.assign(strLine.begin() + 1, strLine.end());
			continue;
		}
		std::string strTemp;
		strTemp.assign(strLine.begin(), strLine.begin() + pos);
		vectorParameters.push_back(strTemp);
		strLine.assign(strLine.begin() + pos, strLine.end());
	}

	if (!strLine.empty())
	{
		vectorParameters.push_back(strLine);
	}

	return 0;
}

static int Print_Function_Usage_Error(std::string strCommand)
{
	printf("function [%s] use error, input \"help [%s]\" for help.\n", strCommand.c_str(), strCommand.c_str());
	return 0;
}

static int Print_Function_Not_Supported(std::string strCommand)
{
	printf("This command is not supported by %s utility.\n", strCommand.c_str());
	return 0;
}


//default to be Dec mode
static int StringToNumber(std::string strContent)
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

static int StringToHEX(std::string strContent, int &number)
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

static int StringToHEX_OneByte(std::string strContent, unsigned char &number)
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
