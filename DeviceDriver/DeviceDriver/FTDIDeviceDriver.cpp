#include "FTDIDeviceDriver.h"
#include "define.h"

#pragma comment(lib, "ftd2xx.lib")
#pragma comment(lib, "TuningChannelDriver.lib")

#include <Windows.h>
#include <fstream>
#include <string>
#include <vector>

#include "TCDDefine.h"
#include "TuningChannelDriver.h"
#include "TCD_FTDICHIP.h"
#include "ftd2xx.h"

namespace {
	enum 
	{
		DEVICEDRIVER_BUFFERSIZE = 512
	};

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

	int SeparateParameters(std::string strLine, std::vector<std::string> &vectorParameters)
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
}

FTDI_DeviceDriver::FTDI_DeviceDriver(void)
{
	m_current_index = -1;
	m_handle_map.clear();
	m_total_Devs = 0;
	m_pTCD = NULL;
	m_lastErrorString.clear();
}


FTDI_DeviceDriver::~FTDI_DeviceDriver(void)
{

}

FTDI_DeviceDriver* FTDI_DeviceDriver::CreateInstance()
{
	if (NULL == _instance)
	{
		_instance = new FTDI_DeviceDriver();
		_instance->ReFresh();
	}
	return _instance;
}

void FTDI_DeviceDriver::CloseInstance()
{
	std::vector<int> vecInt;
	_instance->GetOpened(vecInt);
	for (std::vector<int>::size_type i=0; i != vecInt.size(); ++i)
	{
		_instance->Close(vecInt.at(i));
	}
	
	delete _instance;
	_instance = NULL;
}

FTDI_DeviceDriver* FTDI_DeviceDriver::_instance = NULL;

int FTDI_DeviceDriver::ListAllDevices(int* dev_number)
{
	*dev_number = m_total_Devs;
	return 0;
}

int FTDI_DeviceDriver::List()
{
	//printf("Device number is %d; channel index starts from 0.\n", m_handle_map.size());
	return 0;
}

typedef FT_STATUS (*pfunc_I2C_OpenChannel)(uint32 index, FT_HANDLE *handle);
int FTDI_DeviceDriver::Open(int channel_index)
{
	FT_STATUS ftStatus = FT_OK;
	if (channel_index <0)
	{
		return TCD_PARAM_ERROR;
	}

	std::map<int, TuningChannelDriver*>::iterator iterHandle = m_handle_map.begin();

	iterHandle = m_handle_map.find(channel_index);
	if (!m_handle_map.empty() && m_handle_map.end() != iterHandle)
	{
		return TCD_ALREADY_IN_USE;
	}
	
	////Open a channel in i2c mode.
	FT_HANDLE ftHandle = NULL;
	ftStatus = FT_Open(channel_index, &ftHandle);	
	if (FT_OK != ftStatus)
	{
		return TCD_ALREADY_IN_USE;
	}
  
	//Open successfully.
	m_index_mutex._Lock();
	m_current_index = channel_index;
	m_index_mutex._Unlock();
	TuningChannelDriver* pointer = new TCD_FTDICHIP(ftHandle);

	if (NULL == pointer)
	{
		return TCD_MEMORY_ERROR;
	}

	if (0 != pointer->InitialChannel())
	{
		FT_Close(ftHandle);
		return TCD_FAIL_INITIAL;
	}

	m_handle_map.insert(std::pair<int, TuningChannelDriver*>(channel_index, pointer));
	pointer = NULL;
	return TCD_OK;
}

int FTDI_DeviceDriver::Close(int channel_index)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));
	
	m_pTCD->ReleaseChannel();

	std::map<int, TuningChannelDriver*>::iterator iterTCD;
	iterTCD = m_handle_map.find(channel_index);
	if (m_handle_map.end() != iterTCD)
	{
		m_handle_map.erase(iterTCD);
	}	
	
	delete m_pTCD;
	m_pTCD = NULL;

	m_index_mutex._Lock();
	m_current_index = -1;
	m_index_mutex._Unlock();
	//ftStatus = FT_Close(iterHandle->second);
	return 0;
}

int FTDI_DeviceDriver::LoadConfig(int channel_index, std::string strConfigPath)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));

	return m_pTCD->LoadConfigurationFromFile(strConfigPath);
}

int FTDI_DeviceDriver::SaveConfig(int channel_index, std::string strConfigPath)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));

	return m_pTCD->SaveConfigurationToFile(strConfigPath);
}

//Save file with name serial number.
int FTDI_DeviceDriver::SaveDefaultConfig(int channel_index)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));

	TCDChannelInfo channelInfo;
	m_pTCD->GetChannelInfo(&channelInfo);
	std::string strSerialNumber(channelInfo.serialNumber);

	return m_pTCD->SaveConfigurationToFile(strSerialNumber);
}

int FTDI_DeviceDriver::SetSlaveAddress(int channel_index, unsigned char addr)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));

	return m_pTCD->SetAddress(addr);
}

int FTDI_DeviceDriver::Receive(int channel_index, unsigned char* byteArray, size_t sizeToReceive, size_t *sizeReceived)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));

	uint32 tempvalue;
	m_pTCD->ChannelReceive(byteArray, (uint32)sizeToReceive, &tempvalue);
	*sizeReceived = tempvalue;
	return 0;
}

int FTDI_DeviceDriver::Transfer(int channel_index, std::string strTransferPath)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));

	FILE* hFile;
	fopen_s(&hFile, strTransferPath.c_str(), "rb");

	fseek(hFile, 0, SEEK_SET);
	fseek(hFile, 0, SEEK_END);
	size_t fileLength = ftell(hFile);
	uchar fileBuff[DEVICEDRIVER_BUFFERSIZE];

	
	size_t content_offset = 0;

	//api result
	uint32 result;
	while (content_offset < fileLength) 
	{
		//Check if length with BUFSIZ can be read
		if (content_offset + BUFSIZ < fileLength)
		{
			if (BUFSIZ == fread(fileBuff, sizeof(unsigned char), BUFSIZ, hFile))
			{
					uint32 bytesTransferred = 0;
					result = m_pTCD->ChannelTransfer(fileBuff, sizeof(fileBuff), &bytesTransferred);
					if (0 != result)
					{
						return -1;
					}
			}
			else
			{
				return -1;
			}
		}
		else
		{	
			if ((fileLength - content_offset) == fread(fileBuff, sizeof(unsigned char),fileLength - content_offset, hFile))
			{		
				uint32 bytesTransferred = 0;
				result = m_pTCD->ChannelTransfer(fileBuff, static_cast<uint32>(fileLength - content_offset), &bytesTransferred);
				if (0 != result)
				{
					return -1;
				}
			}
			else
			{
				return -1;
			}
		}
		content_offset += BUFSIZ;
	}

	return TCD_OK;
}

int FTDI_DeviceDriver::Transfer(int channel_index, unsigned char* byteArray, size_t sizeToTransfer, size_t *sizeTransferred)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));

	return m_pTCD->ChannelTransfer(byteArray, (uint32)sizeToTransfer, (puint32)sizeTransferred);	
}

int FTDI_DeviceDriver::ReFresh()
{
	std::vector<int> vectInt;
	std::vector<int>::size_type i = 0;

	GetOpened(vectInt);

	for(; i != vectInt.size(); ++i)
	{
		Close(vectInt.at(i));
	}

	m_handle_map.clear();

	long locIDBuf[16];

	FT_STATUS ftStatus = FT_ListDevices(locIDBuf, &m_total_Devs, FT_LIST_ALL|FT_OPEN_BY_LOCATION);

	if (FT_OK != ftStatus)
	{
		//printf("Fail to refresh devices!\n");
		m_total_Devs = 0;
		return -1;
	}

	return 0;
}

//!Exam if channel index available and if channel is opened.
int FTDI_DeviceDriver::CheckChannel(int channel_index)
{
	std::map<int, TuningChannelDriver*>::iterator iterTCD = m_handle_map.begin();
	iterTCD = m_handle_map.find(channel_index);
	if (m_handle_map.end() == iterTCD)
	{
		//printf("Can not find channel %d!\n", channel_index);
		return TCD_NOT_OPENED;
	}
	
	m_pTCD = iterTCD->second;
	return TCD_OK;
}

int FTDI_DeviceDriver::GetOpened(std::vector<int> &index_vector)
{
	index_vector.clear();
	for (int i=0; i !=m_handle_map.size(); ++i)
	{
		if (NULL != m_handle_map[i])
		{
			index_vector.push_back(i);
		}
	}
	return TCD_OK;
}

int FTDI_DeviceDriver::GetInfo(int channel_index, TCDChannelInfo &channelInfo)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));
	
	return m_pTCD->GetChannelInfo(&channelInfo);;
}

int FTDI_DeviceDriver::SetInfo(int channel_index, const pTCDChannelInfo pchannelInfo)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));

	return m_pTCD->SetChannelInfo(pchannelInfo);
}

int FTDI_DeviceDriver::GetCommonConfig(int channel_index, TCDChannelCommonConfiguration &commonConfig)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));

	return m_pTCD->GetChannelCommonConfiguration(&commonConfig);
}

int FTDI_DeviceDriver::ConfigCommon(int channel_index, const pTCDChannelCommonConfiguration pCommonConfig)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));
	//LOW_CHECK(m_pTCD->CheckChannelCommConfiguration(pCommonConfig, m_lastErrorString));
	return m_pTCD->SaveChannelCommonConfiguration(pCommonConfig);
}

int FTDI_DeviceDriver::GetI2CConfig(int channel_index, TCDChannelI2CConfiguration &I2CConfig)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));

	return m_pTCD->GetChannelI2CConfiguration(&I2CConfig);
}

int FTDI_DeviceDriver::ConfigI2C(int channel_index, const pTCDChannelI2CConfiguration pI2CConfig)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));
	//LOW_CHECK(m_pTCD->CheckChannelI2CConfiguration(pI2CConfig, m_lastErrorString));
	
	return m_pTCD->SaveChannelI2CConfiguration(pI2CConfig);
}

int FTDI_DeviceDriver::GetUARTConfig(int channel_index, TCDChannelUARTConfiguration &UARTConfig)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));

	return m_pTCD->GetChannelUARTConfiguration(&UARTConfig);
}

int FTDI_DeviceDriver::ConfigUART(int channel_index, const pTCDChannelUARTConfiguration pUARTConfig)
{
	FTDI_DEVICE_DRIVER_LOW_CHECK(CheckChannel(channel_index));
	//LOW_CHECK(m_pTCD->CheckChannelUARTConfiguration(pUARTConfig, m_lastErrorString));

	return m_pTCD->SaveChannelUARTConfiguration(pUARTConfig);
}

int FTDI_DeviceDriver::GetLastErrorString(std::string &strErrorString)
{
	strErrorString.clear();
	strErrorString.assign(m_lastErrorString);
	return TCD_OK;
}

int FTDI_DeviceDriver::CloseAll()
{
	std::vector<int> vectInt;
	FTDI_DEVICE_DRIVER_LOW_CHECK(GetOpened(vectInt));

	for (std::vector<int>::size_type i=0; i != vectInt.size(); ++i)
	{
		Close(vectInt.at(i));
	}
	return TCD_OK;
}