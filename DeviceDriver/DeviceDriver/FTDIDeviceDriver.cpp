#include "FTDIDeviceDriver.h"

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
}


FTDI_DeviceDriver::~FTDI_DeviceDriver(void)
{
}

FTDI_DeviceDriver* FTDI_DeviceDriver::CreateInstance()
{
	if (NULL == _instance)
	{
		_instance = new FTDI_DeviceDriver();
	}

	_instance->Reset();
	return _instance;
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

int FTDI_DeviceDriver::Open(int channel_index)
{
	FT_STATUS ftStatus = FT_OK;
	if (channel_index <0)
	{
		return -1;
	}

	std::map<int, TuningChannelDriver*>::iterator iterHandle = m_handle_map.begin();
#if defined SINGLE_CHANNEL_MODE
	for (; iterHandle != m_handle_map.end(); ++iterHandle)
	{
		if (NULL != iterHandle->second && channel_index != iterHandle->first)
		{
			printf("Another channel already opened!\n");
			return 0;
		}
	}
#else
	
#endif

	iterHandle = m_handle_map.find(channel_index);
	if (!m_handle_map.empty() && m_handle_map.end() != iterHandle)
	{
		printf("Already Opened!\n");
		return -1;
	}
	
	//Open a channel
	FT_HANDLE ftHandle = NULL;
	ftStatus = FT_Open(channel_index, &ftHandle);	
	if (FT_OK != ftStatus)
	{
		printf("Open fail!\n");
		return -1;
	}

	//Open successfully.
	m_index_mutex._Lock();
	m_current_index = channel_index;
	m_index_mutex._Unlock();
	TuningChannelDriver* pointer = new TCD_FTDICHIP(ftHandle);

	if (NULL == pointer)
	{
		printf("Open fail!\n");
		return -1;
	}

	m_handle_map.insert(std::pair<int, TuningChannelDriver*>(channel_index, pointer));
	

	if (0 != m_handle_map[channel_index]->InitialChannel())
	{
		printf("Open Channel Fail! Initialization Failed\n");
		return -1;
	}
	return ftStatus;
}

int FTDI_DeviceDriver::Close(int channel_index)
{
	//FT_STATUS ftStatus = FT_OK;
	std::map<int, TuningChannelDriver*>::iterator iterTCD = m_handle_map.begin();
	iterTCD = m_handle_map.find(channel_index);
	if (m_handle_map.end() == iterTCD)
	{
		printf("Can not find channel %d Fail!\n", channel_index);
		return -1;
	}

	
	if (NULL != iterTCD->second)
	{
		iterTCD	->second->ReleaseChannel();
		iterTCD->second = NULL;
		m_handle_map.erase(iterTCD);
	}

	m_index_mutex._Lock();
	m_current_index = -1;
	m_index_mutex._Unlock();
	//ftStatus = FT_Close(iterHandle->second);
	return 0;
}

int FTDI_DeviceDriver::LoadConfig(int channel_index, std::string strConfigPath)
{
	if (0 != CheckChannel(channel_index))
	{
		return -1;
	}

	std::map<int, TuningChannelDriver*>::iterator iterTCD = m_handle_map.begin();
	iterTCD = m_handle_map.find(channel_index);

	int result = iterTCD	->second->LoadConfigurationFromFile(strConfigPath);

	return result;
}

int FTDI_DeviceDriver::SaveConfig(int channel_index, std::string strConfigPath)
{
	if (0 != CheckChannel(channel_index))
	{
		return -1;
	}

	std::map<int, TuningChannelDriver*>::iterator iterTCD = m_handle_map.begin();
	iterTCD = m_handle_map.find(channel_index);


	int result = iterTCD	->second->SaveConfigurationToFile(strConfigPath);

	return result;
}

//Save file with name serial number.
int FTDI_DeviceDriver::SaveDefaultConfig(int channel_index)
{
	if (0 != CheckChannel(channel_index))
	{
		return -1;
	}

	std::map<int, TuningChannelDriver*>::iterator iterTCD = m_handle_map.begin();
	iterTCD = m_handle_map.find(channel_index);

	TCDChannelInfo channelInfo;
	iterTCD->second->GetChannelInfo(&channelInfo);
	std::string strSerialNumber(channelInfo.serialNumber);
	int result = iterTCD	->second->SaveConfigurationToFile(strSerialNumber);

	return result;
}

int FTDI_DeviceDriver::SetAddress(int channel_index, unsigned char addr)
{
	if (0 != CheckChannel(channel_index))
	{
		return -1;
	}
	std::map<int, TuningChannelDriver*>::iterator iterTCD = m_handle_map.begin();
	iterTCD = m_handle_map.find(channel_index);
	iterTCD->second->SetAddress(addr);

	return 0;
}


int FTDI_DeviceDriver::Receive(int channel_index, std::string strRecPath)
{
	if (0 != CheckChannel(channel_index))
	{
		return -1;
	}

	std::fstream fsOutput;
	fsOutput.open(strRecPath.c_str(), std::ios::out || std::ios::trunc || std::ios::binary);
	if (!fsOutput.is_open())
	{
		return -1;
	}

	std::map<int, TuningChannelDriver*>::iterator iterTCD = m_handle_map.begin();
	iterTCD = m_handle_map.find(channel_index);
	if (m_handle_map.end() == iterTCD)
	{
		return -1;
	}

	uchar buffer[DEVICEDRIVER_BUFFERSIZE];
	uint32 bytesReceived = -1;
	while ((0 ==iterTCD->second->ChannelReceive(buffer, DEVICEDRIVER_BUFFERSIZE, &bytesReceived)) && 0 != bytesReceived)
	{
		std::string strContent;
		for (uint32 i=0; i != bytesReceived && i != DEVICEDRIVER_BUFFERSIZE; ++i)
		{
			strContent.push_back(buffer[i]);
		}
		fsOutput<<strContent;
	}
	
	return 0;
}

int FTDI_DeviceDriver::Receive(int channel_index, unsigned char* byteArray, size_t sizeToReceive, size_t *sizeReceived)
{
	if (0 != CheckChannel(channel_index))
	{
		return -1;
	}
	
	std::map<int, TuningChannelDriver*>::iterator iterTCD = m_handle_map.begin();
	iterTCD = m_handle_map.find(channel_index);

	if (m_handle_map.end() == iterTCD)
	{
		return -1;
	}
	
	uint32 tempvalue;
	iterTCD->second->ChannelReceive(byteArray, (uint32)sizeToReceive, &tempvalue);
	*sizeReceived = tempvalue;
	return 0;
}

int FTDI_DeviceDriver::Transfer(int channel_index, std::string strTransferPath)
{
	if (0 != CheckChannel(channel_index))
	{
		return -1;
	}

	FILE* hFile;
	fopen_s(&hFile, strTransferPath.c_str(), "rb");

	fseek(hFile, 0, SEEK_SET);
	fseek(hFile, 0, SEEK_END);
	size_t fileLength = ftell(hFile);
	uchar fileBuff[DEVICEDRIVER_BUFFERSIZE];

	
	size_t content_offset = 0;

	std::map<int, TuningChannelDriver*>::iterator iterTCD = m_handle_map.begin();
	iterTCD = m_handle_map.find(channel_index);
	if (m_handle_map.end() == iterTCD)
	{
		return -1;
	}

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
					result = iterTCD->second->ChannelTransfer(fileBuff, sizeof(fileBuff), &bytesTransferred);
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
				result = iterTCD->second->ChannelTransfer(fileBuff, static_cast<uint32>(fileLength - content_offset), &bytesTransferred);
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


	return 0;
}

int FTDI_DeviceDriver::Transfer(int channel_index, unsigned char* byteArray, size_t sizeToTransfer, size_t *sizeTransferred)
{
	if (0 != CheckChannel(channel_index))
	{
		return -1;
	}
	std::map<int, TuningChannelDriver*>::iterator iterTCD = m_handle_map.begin();
	iterTCD = m_handle_map.find(channel_index);
	if (m_handle_map.end() == iterTCD)
	{
		return -1;
	}

	iterTCD->second->ChannelTransfer(byteArray, (uint32)sizeToTransfer, (puint32)sizeTransferred);

	return 0;
}
//int DeviceDriver::Call(std::vector<std::string> string_vector)
//{
//	
//	if (string_vector.empty())
//	{
//		//No parameters
//		return -1;
//	}
//
//	int func_result = 0;
//	//with two
//	FUNC_TYPE current_func = static_cast<FUNC_TYPE>(string_vector.size() - 1);
//
//	switch(current_func)
//	{
//	case INPUT_PARAM_0:
//		{
//			std::map<std::string ,fun >::iterator iter_fun;; 
//			if (funMAP.end() != (iter_fun = funMAP.find(string_vector.at(0))))
//			{
//				func_result = (this->*funMAP[string_vector.at(0)])();
//				break;
//			} 
//			print_help();
//		}
//		break;
//	case INPUT_PARAM_1:
//		{
//			std::map<std::string ,funA >::iterator iter_funA;; 
//			if (funAMAP.end() != (iter_funA = funAMAP.find(string_vector.at(0))))
//			{
//				func_result = (this->*funAMAP[string_vector.at(0)])(StringToNumber(string_vector.at(1)));
//				break;
//			}
//
//			std::map<std::string ,funS >::iterator iter_funS;; 
//			if (funSMAP.end() != (iter_funS = funSMAP.find(string_vector.at(0))))
//			{
//				func_result = (this->*funSMAP[string_vector.at(0)])(string_vector.at(1));
//				break;
//			}
//			print_help();
//		}
//		break;
//	case INPUT_PARAM_2:
//		{
//			std::map<std::string ,funB >::iterator iter_funB;; 
//			if (funBMAP.end() != (iter_funB = funBMAP.find(string_vector.at(0))))
//			{
//				func_result = (this->*funBMAP[string_vector.at(0)])(StringToNumber(string_vector.at(1)), string_vector.at(2));
//				break;
//			}
//			print_help();
//		}
//		break;		
//	default:
//		printf("No functions are found or parameters not available");
//		print_help();
//		return -1;
//	}
//	if (0 != func_result)
//	{
//		print_help();
//	}
//	//
//	return 0;
//}

//int DeviceDriver::Call(std::string strLine)
//{
//	std::vector<std::string> vector_parameters;
//	SeparateParameters(strLine, vector_parameters);
//
//	return (Call(vector_parameters));
//}

//int DeviceDriver::Help()
//{
	////printf("\nFunction Help is Called!\n");
	//printf("\nThe flowing functions are available\n");
	////printf("")
	//std::map<std::string, fun>::iterator iter_FUNMAP = funMAP.begin();
	//std::map<std::string, funA>::iterator iter_FUNAMAP = funAMAP.begin();
	//std::map<std::string, funB>::iterator iter_FUNBMAP = funBMAP.begin();
	////for (size_t i=0; i != funMAP.size(); ++i)
	////{
	////	printf("%s", funMAP[i]->first);
	////}
	//for (; iter_FUNMAP != funMAP.end(); ++iter_FUNMAP)
	//{
	//	printf("%s\n",iter_FUNMAP->first.c_str());
	//}

	//for (; iter_FUNAMAP != funAMAP.end(); ++iter_FUNAMAP)
	//{
	//	printf("%s\n",iter_FUNAMAP->first.c_str());
	//}

	//for (; iter_FUNBMAP != funBMAP.end(); ++iter_FUNBMAP)
	//{
	//	printf("%s\n",iter_FUNBMAP->first.c_str());
	//}

//	return 0;
//}

//int DeviceDriver::HelpFunc(std::string strFuncName)
//{
//	printf("Function: %s help is called!\n", strFuncName.c_str());
//	return 0;
//}

int FTDI_DeviceDriver::Reset()
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
		printf("Fail to refresh devices!\n");
		m_total_Devs = 0;
		return -1;
	}

	return 0;
}

int FTDI_DeviceDriver::CheckChannel(int channel_index)
{
	std::map<int, TuningChannelDriver*>::iterator iterTCD = m_handle_map.begin();
	iterTCD = m_handle_map.find(channel_index);
	if (m_handle_map.end() == iterTCD)
	{
		printf("Can not find channel %d!\n", channel_index);
		return -1;
	}

	if (NULL == iterTCD->second)
	{
		printf("Channel is %d not opened!\n", channel_index);
		return -2;
	}
	return 0;
}

int FTDI_DeviceDriver::GetOpened(std::vector<int> &index_vector)
{
	index_vector.clear();
	std::map<int, TuningChannelDriver*>::iterator iterTCD = m_handle_map.begin();

	for (int i=0; i !=m_handle_map.size(); ++i)
	{
		if (NULL != m_handle_map[i])
		{
			index_vector.push_back(i);
		}
	}
	return 0;
}