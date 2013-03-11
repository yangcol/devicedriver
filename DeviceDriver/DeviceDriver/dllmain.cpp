#include <Windows.h>
#include "CallDeviceDriver.h"
extern int CallDeviceDriver(std::string strCommand);
int UseDeviceDriver(std::string *strCommand)
{
	CallDeviceDriver(*strCommand);
	return 0;
}

BOOL WINAPI DllMain(
	HANDLE hinstDLL, 
	DWORD dwReason, 
	LPVOID lpvReserved)
{
	return TRUE;
}
