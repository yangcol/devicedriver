//!@file CallDeviceDriver.cpp
//!@brief Process commands to call device driver functions.
//!@remark Now only one channel is allowed to be used. But notice that,
//device driver is able to manage all devices, so it's able to manage several devices one time. If you want to
//use more than one devices, modify codes in this CPP file.
#include "FTDIDeviceDriver.h"
#include "TCDDefine.h"
#include "XMLParser.h"
#include "define.h"
#include <string>

#include "Util.h"
#ifndef BATCH_TEST
#define BATCH_TEST
#include <iostream>
#include <fstream>
#endif

int Init_Call_DeviceDriver();
int CallDeviceDriver(std::string strCommand);

using namespace::TCDNameSpace::TCDConfiguration;

namespace {
	const std::string static_strXML = "funclist.xml";
	typedef int (*fun)();
	typedef std::map<std::string, fun>::iterator iterFun;
	std::map<std::string, fun> static_function_MAP;
	const std::string const_help_string = "Input [help] for more information\n";
	const std::string const_not_found_string = " not recognized";
	FTDI_DeviceDriver* static_pDeviceDriver = NULL;
	std::vector<std::string> static_vector_command;
	std::vector<int> static_open_vector;
	bool static_bInitializedFlag = false;
	static unsigned char readByte = 0;
	int static_current_open_index = -1;
	enum 
	{
		DEFAUL_OPEN_INDEX = 0
	};
	//static_vector_command include function name, so if with no input param, size of command is 1.
	//INPUT_PARAM_0 identifies the command index of command.
	enum 
	{
		INDEX_COMMAND_NAME = 0,
		INPUT_PARAM_0,
		INPUT_PARAM_1,
		INPUT_PARAM_2,
		INPUT_PARAM_3,
		INPUT_PARAM_4,
		INPUT_PARAM_5,
		INPUT_PARAM_6
	};

	enum {
		RATE_CLOCKRATE_K = 1000
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

static int call_configi2c();
static int call_geti2c();

static int call_configcom();
static int call_getcom();

static int call_configuart();
static int call_getuart();


static int call_quit();

static int Print_Function_Usage_Error();
static int Print_Function_Not_Supported();

static int GetOpened();

int CallDeviceDriver(std::string strCommand)
{
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

	iterFunctionMap = static_function_MAP.find(static_vector_command.at(INDEX_COMMAND_NAME));

	if (static_function_MAP.end() == iterFunctionMap)
	{
		//Function with name is not found in function list
		printf("[%s] %s, %s", static_vector_command.at(INDEX_COMMAND_NAME).c_str(), const_not_found_string.c_str(), const_help_string.c_str());
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

int Init_Call_DeviceDriver()
{
	static_function_MAP.clear();

	//Reg all functions
	FTDI_DEVICE_DRIVER_LOW_CHECK(RegFunc("help", call_help));
	FTDI_DEVICE_DRIVER_LOW_CHECK(RegFunc("list", call_list));
	RegFunc("open", call_open);
	RegFunc("close", call_close);
	RegFunc("load", call_load);
	RegFunc("save", call_save);
	RegFunc("send", call_send);
	RegFunc("receive", call_receive);

	RegFunc("confi2c", call_configi2c);
	RegFunc("geti2c", call_geti2c);

	RegFunc("confcom", call_configcom);
	RegFunc("getcom", call_getcom);

	RegFunc("confuart", call_configuart);
	RegFunc("getuart", call_getuart);

	RegFunc("quit", call_quit);
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
	enum {
		FUNCNAME = 1//index of function name called for help on static_vector_command
	};

	switch (static_vector_command.size())
	{
		//Others
		//help + 0 params
	case INPUT_PARAM_0:
		{
			//list all functions
			iterFun iterFunctionMap = static_function_MAP.begin();
			printf("Please use space to separate your parameters.\n");
			printf("Available functions are listed below:\n");
			//Get functions from xml
			for (; iterFunctionMap != static_function_MAP.end(); ++iterFunctionMap)
			{
				printf("\t%s\n", iterFunctionMap->first.c_str());
			}
		}
		break;	
		//help + 1 params
	case INPUT_PARAM_1:
		{
			std::string strfuncDetail;
			iterFun iterFunctionMap = static_function_MAP.find(static_vector_command.at(FUNCNAME));
			if (static_function_MAP.end() == iterFunctionMap)
			{
				Print_Function_Not_Supported();
			}
			DeviceDriver_XMLParser xmlParser;
			FTDI_DEVICE_DRIVER_LOW_CHECK(xmlParser.LoadXML(static_strXML));
			FTDI_DEVICE_DRIVER_LOW_CHECK(xmlParser.GetFunctionDetail(static_vector_command.at(FUNCNAME), strfuncDetail));
			std::cout<<strfuncDetail.c_str()<<std::endl;
		}
		break;
	default:
		Print_Function_Usage_Error();
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
		Print_Function_Usage_Error();
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
			result = static_pDeviceDriver->Open(StringToNumber(static_vector_command.at(1)));
		}
		break;
	default:
		Print_Function_Usage_Error();
		return -1;
	}

	if (0 == result)
	{
		printf("Success!\n");
	} else
	{
		printf("Fail!\n");
	}

	return result;
}

//Close all opened devices
static int call_close()
{
	int result  = -1;
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_0:
		{
			static_pDeviceDriver->CloseAll();
		}
		break;
	case INPUT_PARAM_1:
		{
			int res = StringToNumber(static_vector_command.at(1));
			if (-1 == res)
			{
				printf("Input params error!\n");
				break;
			}
			static_pDeviceDriver->Close(res);
			break;
		}
	default:
		Print_Function_Usage_Error();
		return -1;
	}

	return result;
}

static int call_load()
{
	int result  = -1;
	FTDI_DEVICE_DRIVER_LOW_CHECK(GetOpened());
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_1:
		{
			std::vector<int> vector_int;
			
			result = static_pDeviceDriver->LoadConfig(static_current_open_index, static_vector_command.at(1));
		}
		break;
	case INPUT_PARAM_2:
		result = static_pDeviceDriver->LoadConfig(StringToNumber(static_vector_command.at(1)), static_vector_command.at(2));
		break;
	default:
		Print_Function_Usage_Error();
		return -1;
	}

	return result;
}

static int call_save()
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(GetOpened());
	int result  = -1;
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_0:
		{
			result = static_pDeviceDriver->SaveDefaultConfig(static_current_open_index);
		}
		break;
	case INPUT_PARAM_1:
		{
			result = static_pDeviceDriver->SaveConfig(static_current_open_index, static_vector_command.at(1));
		}
		break;
	default:
		result = -1;
		break;
	}
	
	if (0 != result)
	{
		Print_Function_Usage_Error();
	}
	return result;
}

//Command send act as
//send [addr] [para1] [para2] ...
//separated by space
static int call_send()
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(GetOpened());
	int result  = -1;
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_0:
	case INPUT_PARAM_1:
		Print_Function_Not_Supported();
		break;
	default:
		{
			const int paramOffSet = 2;	
			const int commandOffset = 1;
			unsigned char addr = 0;
			if (0 != StringToHEX_OneByte(static_vector_command.at(1), addr))
			{
				//printf("Input params error!\n");
				Print_Function_Usage_Error();
				return -1;
			}

			unsigned char writeByte = 0;
			size_t sizeWritten;
			static_pDeviceDriver->SetSlaveAddress(static_current_open_index, addr);
			// in command vector, bytes starts from index 2. 0 is command, 1 is address.
			unsigned char *buffer = new unsigned char[static_vector_command.size() - paramOffSet];
			for (size_t i=0; i != static_vector_command.size() - paramOffSet; ++i)
			{
    			if (0 != StringToHEX_OneByte(static_vector_command.at(i + paramOffSet), buffer[i]))
				{
                    
					//printf("Input params error!\n");
					Print_Function_Usage_Error();
					return -1;
				}
			}

			int result = -1;
			result = static_pDeviceDriver->Transfer(static_current_open_index, buffer, static_vector_command.size() - paramOffSet, &sizeWritten);
			delete buffer;
			if (0 != result)
			{
				printf("Transfer Failed!\n");
			}
		}
		break;
	}
	return 0;
}

static int call_receive()
{
	enum{
		DEFAULT_RECEIVE = 1
	};

	FTDI_DEVICE_DRIVER_LOW_CHECK(GetOpened());

	switch(static_vector_command.size())
	{
	case INPUT_PARAM_1:
	case INPUT_PARAM_2:
		{
			unsigned char addr = 0;
			if (0 != StringToHEX_OneByte(static_vector_command.at(INPUT_PARAM_0), addr))
			{
				//printf("Input params error!\n");
				Print_Function_Usage_Error();
				return -1;
			}

			unsigned char *recBuffer;
			size_t sizeReceived = 0;
			static_pDeviceDriver->SetSlaveAddress(static_current_open_index, addr);

			unsigned char number = DEFAULT_RECEIVE;
			if (INPUT_PARAM_2 == static_vector_command.size())
			{		
				if (0 != StringToHEX_OneByte(static_vector_command.at(INPUT_PARAM_1), number))
				{
					Print_Function_Usage_Error();
					//printf("Input params error!\n");
					return -1;
				}
			}

			try
			{
				recBuffer = new unsigned char[number];
			}
			catch (std::bad_alloc)
			{
				printf("Memory Error!\n");	
			}

			static_pDeviceDriver->Receive(static_current_open_index, recBuffer, number, &sizeReceived);
			printf("Receive %d Values: ", number);
			for (unsigned char i=0; i != number; ++i)
			{
				printf("%X\n", recBuffer[i]);
			}
			delete recBuffer;
		}
		break;
	default:
		{	
			Print_Function_Usage_Error();
		}
	}
	return 0;
}

//!Config i2c
//!@param 1 clockrate 100 identifies 100k
//!@param 2 sendOption	
//!@param 3 receiveOption
//#define	I2C_TRANSFER_OPTIONS_START_BIT		0x00000001

/*Generate stop condition before transmitting */
//#define I2C_TRANSFER_OPTIONS_STOP_BIT		0x00000002

/*Continue transmitting data in bulk without caring about Ack or nAck from device if this bit is 
not set. If this bit is set then stop transitting the data in the buffer when the device nAcks*/
//#define I2C_TRANSFER_OPTIONS_BREAK_ON_NACK	0x00000004

/* libMPSSE-I2C generates an ACKs for every byte read. Some I2C slaves require the I2C 
master to generate a nACK for the last data byte read. Setting this bit enables working with such 
I2C slaves */
//#define I2C_TRANSFER_OPTIONS_NACK_LAST_BYTE	0x00000008
//!
//Example: confi2c 100 7 10
//Config i2c + clockrate
static int call_configi2c()
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(GetOpened());
	enum {
		CLOCKRATE = 1,
		SEND_OPTION,
		RECEIVE_OPTION
	};
	int result = 0;
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_3:
		{
			TCDChannelI2CConfiguration channelI2CConfig;
			channelI2CConfig.clockrate = RATE_CLOCKRATE_K * StringToNumber(static_vector_command.at(CLOCKRATE)); // param 1 clock rate
			channelI2CConfig.sendOption = StringToNumber(static_vector_command.at(SEND_OPTION));
			channelI2CConfig.receiveOption = StringToNumber(static_vector_command.at(RECEIVE_OPTION));
			int res = static_pDeviceDriver->ConfigI2C(static_current_open_index, &channelI2CConfig);
			std::string strErrorString;
			if (0 != res)
			{
				static_pDeviceDriver->GetLastErrorString(strErrorString);
			}
			printf("%s", strErrorString.c_str());
		}
		break;
	default:
		Print_Function_Usage_Error();
		return -1;
	}
	return 0;
}


static int call_geti2c()
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(GetOpened());
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_0:
		{
			TCDChannelI2CConfiguration channelI2CConfig;
			int res = static_pDeviceDriver->GetI2CConfig(static_current_open_index, channelI2CConfig);
			printf("Clock Rate = %d K\nSend Option = %d\nReceive Option = %d\nThree Phase Clock = %d\n", \
				channelI2CConfig.clockrate/RATE_CLOCKRATE_K,\
				channelI2CConfig.sendOption,
				channelI2CConfig.receiveOption,
				(int)(channelI2CConfig.threephaseclock));	
		}
		break;
	default:
		Print_Function_Usage_Error();
		break;
	}
	return 0;
}

//!Config common configuration.
//@param 1 USBBuffer
//@param 2 Read time outs
//@param 3 Write time outs
//@param 4 Retry count
//@param 5 Latency time
static int call_configcom()
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(GetOpened());
	enum {
		USB_BUFFER = 1,
		READ_TIME_OUTS,
		WRITE_TIME_OUTS,
		RETRY_COUNT,
		LATENCY_TIME
	};
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_5:
		{
			TCDChannelCommonConfiguration commonConfig;
			commonConfig.USBBuffer = StringToNumber(static_vector_command.at(USB_BUFFER));
			commonConfig.readtimeouts = StringToNumber(static_vector_command.at(READ_TIME_OUTS));
			commonConfig.writetimeouts = StringToNumber(static_vector_command.at(WRITE_TIME_OUTS));
			commonConfig.retrycount = StringToNumber(static_vector_command.at(RETRY_COUNT));
			commonConfig.latencytime = StringToNumber(static_vector_command.at(LATENCY_TIME));
			int res = static_pDeviceDriver->ConfigCommon(static_current_open_index, &commonConfig);
			if (0 != res)
			{
				std::string strTemp;
				static_pDeviceDriver->GetLastErrorString(strTemp);
				printf("%s", strTemp);
			}
		}
		break;
	default:
		Print_Function_Usage_Error();
		break;
	}

	return 0;
}

static int call_getcom()
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(GetOpened());
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_0:
		{
			TCDChannelCommonConfiguration channeCommConfig;
			int res = static_pDeviceDriver->GetCommonConfig(static_current_open_index, channeCommConfig);
			printf("USB Buffer = %d \nRead Time Outs = %d\nWrite Time Outs = %d\nRetry Count = %d\nLatency Time = %d\n", \
				channeCommConfig.USBBuffer,\
				channeCommConfig.readtimeouts,
				channeCommConfig.writetimeouts,
				(int)(channeCommConfig.retrycount),
				channeCommConfig.latencytime);	
		}
		break;
	default:
		Print_Function_Usage_Error();
		break;
	}

	return 0;
}

//!Config UART Configuration
//@param 1 Baud rate
static int call_configuart()
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(GetOpened());
	
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_5:
		break;
	default:
		Print_Function_Usage_Error();
		break;

	}
	return 0;
}

static int call_getuart()
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(GetOpened());
	int res = -1;
	TCDChannelUARTConfiguration channeUARTConfig;
	switch(static_vector_command.size())
	{
	case INPUT_PARAM_0:
		res = static_pDeviceDriver->GetUARTConfig(static_current_open_index, channeUARTConfig);	
		break;
	default:
		res = -1;
		break;
	}
	if (0 != res)
	{
		Print_Function_Usage_Error();
	} else
	{
		printf("Baud Rate = %d \nBit Per Word = %d\nStop Bits = %d\nParity = %d\nFlow Control = %d\n", \
			channeUARTConfig.baudrate,
			channeUARTConfig.bitsperword,
			channeUARTConfig.stopbits,
			channeUARTConfig.parity,
			channeUARTConfig.flowcontrol);	
	}

	return res;
}

static int call_quit()
{
	static_pDeviceDriver->CloseInstance();
	return 0;
}

static int Print_Function_Usage_Error()
{
	printf("function [%s] use error, input \"help [%s]\" for help.\n", static_vector_command.at(INDEX_COMMAND_NAME).c_str(), static_vector_command.at(INDEX_COMMAND_NAME).c_str());
	return 0;
}

static int Print_Function_Not_Supported()
{
	printf("This command is not supported by %s utility.\n", static_vector_command.at(INDEX_COMMAND_NAME).c_str());
	return 0;
}

static int GetOpened()
{
	static_pDeviceDriver->GetOpened(static_open_vector);
	if (0 == static_open_vector.size())
	{
		printf("No device opened!\n");
		return -1;
	}
	static_current_open_index = static_open_vector.at(DEFAUL_OPEN_INDEX);
	return 0;
}

//On multi-thread, this function may not good enough