#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "FTDIDeviceDriver.h"

//#include "vld.h"
using std::cin;

extern int Init_Call_DeviceDriver();
extern int CallDeviceDriver(std::string strCommand);
extern int StringToHEX_OneByte(std::string strContent);

#define MAX_CHARS_PER_LINE 64
int main(int argc, char* argv[])
{
	char char_input[MAX_CHARS_PER_LINE];
	std::vector<std::string> vectorParameters;
	printf("Device Driver Now is Running!\n");
	if (0 != Init_Call_DeviceDriver())
	{
		return -1;
	}

	if (argc == 1)
	{
		while (cin.getline(char_input, sizeof(char_input)))
		{
			std::string strCommand(char_input);
			CallDeviceDriver(strCommand);
			//{
				//printf("Command [%s] Failed ! Test Failed!\n", strCommand.c_str());
				//return -1;
			//}
			if (0 == strCommand.compare("quit"))
			{
				return 0;
			}

		}
	}

	if (argc == 2)
	{
		std::string inputFileName(argv[1]);
		/*FILE* pfile = fopen(inputFileName.c_str(), "r");*/
		std::fstream fs;
		fs.open(inputFileName.c_str(), std::ios::in | std::ios::_Nocreate);
		if (!fs.is_open())
		{
			printf("Error! File not exists!\n");
			return -1;
		}
		
		while(fs.getline(char_input, sizeof(char_input)))
		{
			std::string strCommand(char_input);

			CallDeviceDriver(strCommand);

			if (0 == strCommand.compare("quit"))
			{
				return 0;
			}

		}

	}

	return 0;
}


