#pragma once
#include <string>
#include <map>
#include <vector>

class TuningChannelDriver;
//Single instance of device driver
class FTDI_DeviceDriver
{
public:
	static FTDI_DeviceDriver* CreateInstance();

	~FTDI_DeviceDriver(void);


	int Open(int channel_index);
	int ListAllDevices(int* dev_number);
	int GetOpened(std::vector<int> &index_vector);
	int List();
	//Close current opened 
	//int Close();
	int Close(int channel_index);

	int LoadConfig(int channel_index, std::string strConfigPath);
	int SaveConfig(int channel_index, std::string strConfigPath);
	int SaveDefaultConfig(int channel_index);

	int Receive(int channel_index, std::string strRecPath);
	int Transfer(int channel_index, std::string strTransferPath);

	int Receive(int channel_index, unsigned char* byteArray, size_t sizeToTransfer, size_t *sizeTransferred);
	int Transfer(int channel_index, unsigned char* byteArray, size_t sizeToTransfer, size_t *sizeTransferred);

	int Reset();
	int SetAddress(int channel_index, unsigned char addr);

	int ConfigCommon(int channel_index);
	int ConfigI2C(int channel_index);
	int ConfigUART(int channel_index);
private:
	
	static FTDI_DeviceDriver* _instance;
	FTDI_DeviceDriver(const FTDI_DeviceDriver &other);
	FTDI_DeviceDriver & FTDI_DeviceDriver::operator =(const FTDI_DeviceDriver &other);
	FTDI_DeviceDriver(void);

	std::map<int, TuningChannelDriver*>m_handle_map;
	int CheckChannel(int channel_index);
	/*int current_index = -1;*/
	std::_Mutex m_index_mutex;
	int m_current_index;
	int m_total_Devs;
};

