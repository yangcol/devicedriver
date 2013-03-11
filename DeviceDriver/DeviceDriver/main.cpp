#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>
#include <fstream>
#include "FTDIDeviceDriver.h"
using std::cin;

extern int CallDeviceDriver(std::string strCommand);
extern int StringToHEX_OneByte(std::string strContent);

#define MAX_CHARS_PER_LINE 64
int main(int argc, char* argv[])
{
	char char_input[MAX_CHARS_PER_LINE];
	std::vector<std::string> vectorParameters;
	if (argc == 1)
	{
		printf("Device Driver Now is Running!\n");
		//std::string strInput(0);


		std::string strLine(" Device  Driver Now is Running!");

		while (cin.getline(char_input, sizeof(char_input)))
		{
			std::string strCommand(char_input);
			if (0 == strCommand.compare("quit"))
			{
				return 0;
			}

		CallDeviceDriver(strCommand);
			//{
				//printf("Command [%s] Failed ! Test Failed!\n", strCommand.c_str());
				//return -1;
			//}
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
			if (0 == strCommand.compare("quit"))
			{
				return 0;
			}

			if (0 != CallDeviceDriver(strCommand))
			{
				printf("Command [%s] Failed ! Test Failed!\n", strCommand.c_str());
				return -1;
			}
		}

	}

	return 0;
}


