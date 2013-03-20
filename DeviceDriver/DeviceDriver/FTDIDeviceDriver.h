//!@file FTDIDeviceDriver.h
//!@brief Class FTDI_DeviceDriver is used to manage all devices.
#pragma once
#include <string>
#include <map>
#include <vector>
#include "TCDDefine.h"

using TCDNameSpace::TCDChannelInfo;
using namespace TCDNameSpace::TCDConfiguration;

class TuningChannelDriver;
//Single instance of device driver
class FTDI_DeviceDriver
{
public:
	static FTDI_DeviceDriver* CreateInstance();
	static void CloseInstance();
	~FTDI_DeviceDriver(void);
	

	int Open(int channel_index);
	int ListAllDevices(int* dev_number);
	int GetOpened(std::vector<int> &index_vector);
	int List();
	//Close current opened 
	//int Close();
	int Close(int channel_index);
	int CloseAll();

	//!Load configuration from local file.
	int LoadConfig(int channel_index, std::string strConfigPath);
	int SaveConfig(int channel_index, std::string strConfigPath);
	int SaveDefaultConfig(int channel_index);

	//int Receive(int channel_index, std::string strRecPath);
	int Transfer(int channel_index, std::string strTransferPath);

	int Receive(int channel_index, unsigned char* byteArray, size_t sizeToTransfer, size_t *sizeTransferred);
	int Transfer(int channel_index, unsigned char* byteArray, size_t sizeToTransfer, size_t *sizeTransferred);

	int ReFresh();
	int SetSlaveAddress(int channel_index, unsigned char addr);
	
	
	
	/*char manufactureID[16];	///<Manufacture ID.
	char manufacture[32];		///<Manufacture.
	char description[64];			///<Description.
	char serialNumber[16];		///<Serial number.*/
	
	//!Use channel info.
	//!@param channel_index [in] channel index
	//!@param manufactureID [in] manufactureID
	int GetInfo(int channel_index, TCDChannelInfo &channelInfo);

	int SetInfo(int channel_index, const pTCDChannelInfo pchannelInfo);

	int GetCommonConfig(int channel_index, TCDChannelCommonConfiguration &commonConfig);
	int ConfigCommon(int channel_index, const pTCDChannelCommonConfiguration pCommonConfig);
	
	//Clock rate
	int GetI2CConfig(int channel_index, TCDChannelI2CConfiguration &I2CConfig);
	int ConfigI2C(int channel_index, const pTCDChannelI2CConfiguration pI2CConfig);
	
	int GetUARTConfig(int channel_index, TCDChannelUARTConfiguration &UARTConfig);
	int ConfigUART(int channel_index, const pTCDChannelUARTConfiguration pUARTConfig);
	
	int GetLastErrorString(std::string &strErrorString);
private:
	
	static FTDI_DeviceDriver* _instance;
	FTDI_DeviceDriver(const FTDI_DeviceDriver &other);
	FTDI_DeviceDriver & FTDI_DeviceDriver::operator =(const FTDI_DeviceDriver &other);
	FTDI_DeviceDriver(void);

	std::map<int, TuningChannelDriver*>m_handle_map;
	TuningChannelDriver* m_pTCD;
	int CheckChannel(int channel_index);
	/*int current_index = -1;*/
	std::_Mutex m_index_mutex;
	int m_current_index;
	int m_total_Devs;
	std::string m_lastErrorString;
};
