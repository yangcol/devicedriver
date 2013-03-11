#include "TCD_FTDICHIP.h"
#include <Windows.h>
#include <time.h>
#include "TCDDefine.h"
#include "TCD_FTDICHIP_ChannelCommunication.h"
#include "TCD_FTDICHIP_ChannelConfiguration.h"

#include "ftd2xx.h"

using namespace TCDNameSpace::TCDConfiguration;

static bool CheckStateOperation(int channelState, int operation);

namespace {
	enum	//<Channel State 
	{
		CHANNELSTATE_NOT_INITIALED,
		CHANNELSTATE_READY,
		CHANNELSTATE_RELEASED
	};

	enum //<Corresponding Operation
	{
		OPERATE_INIT,
		OPERATE_RESET,
		OPERATE_RELEASE,
		OPERATE_CONFIG,
		OPERATE_COMM,
		OPERATE_QUERY
	};
}

//!Data struct used by TCD.
struct TCD_FTDICHIP::TCDDataStruct
{

	TCDChannelInfo channelInfo;
	uint32 locId;							//<Local id which is composed with USB port.
	uint32 flag;							//<Indicate if channel is in use, initial
	uint32 type;							//<Indicate which type of chip is.
	TCD_FTDICHIP_ChannelCommunication* pCommunication;	//<Communication pointer.
	TCD_FTDICHIP_ChannelConfiguration* pConfiguration;		//<Configuration pointer.
	int channelState;					//<Channel state.
	char configFilePath[MAX_PATH_DEPTH];
	CRITICAL_SECTION m_channelSLock;
} tcdDataInstance;

TCD_FTDICHIP::TCD_FTDICHIP(TCDHandle tcdHandle)
{
	m_tcdData = &tcdDataInstance;
	m_tcdData->channelState = CHANNELSTATE_NOT_INITIALED;

	m_tcdHandle = tcdHandle;
	//Create communication and configuration
	m_tcdData->pCommunication = new TCD_FTDICHIP_ChannelCommunication(m_tcdHandle);
	m_tcdData->pConfiguration = new TCD_FTDICHIP_ChannelConfiguration(m_tcdHandle);
	//Check TCD State.
	InitializeCriticalSection(&m_tcdData->m_channelSLock);
}

TCD_FTDICHIP::~TCD_FTDICHIP(void)
{
	FT_Close(m_tcdHandle);
}

////!Check if channel is initialized.
//bool TCD_FTDICHIP::IsInitialized()
//{
//	//Check channel handle.
//	if (NULL == m_tcdHandle)
//	{
//		return false;
//	}
//
//	//Check private data struct.
//	if (NULL == m_tcdData)
//	{
//		return false;
//	}
//
//	//Check channel state.
//	if (m_tcdData->channelState == CHANNELSTATE_NOT_INITIALED)
//	{
//		return false;
//	}
//	return true;
//}

uint32 TCD_FTDICHIP::InitialChannel()
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_INIT))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}
	
	//According to handle get from 
	int iDevice = 0;
	FT_STATUS ftStatus = 0;


	//
	if (0 != RefreshChannelInfo())
	{
		returnValue = TCD_FAIL_TO_READEE;
		goto _end;
	}

	uint32 initial_config_comm = 0;
	uchar channel_format = -1;
	uchar address = 0;
	initial_config_comm = m_tcdData->pConfiguration->Initial();
	initial_config_comm |= m_tcdData->pConfiguration->GetChannelFormat(&channel_format);
	initial_config_comm |= m_tcdData->pConfiguration->GetAddress(&address);
	
	initial_config_comm |= m_tcdData->pCommunication->Initial();
	initial_config_comm |= m_tcdData->pCommunication->SetFormat(channel_format);
	initial_config_comm |= m_tcdData->pCommunication->SetAddress(address);
	if (TCD_OK != initial_config_comm)
	{
		returnValue = TCD_FAIL_INITIAL;
		goto _end;
	}

	m_tcdData->channelState = CHANNELSTATE_READY;	

_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

//use default configuration 
uint32 TCD_FTDICHIP::ResetChannel()
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_RESET))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	if (0 != RefreshChannelInfo())
	{
		returnValue = TCD_FAIL_TO_READEE;
		goto _end;
	}

	m_tcdData->pCommunication->Reset();
	m_tcdData->pConfiguration->Reset();

	m_tcdData->channelState = CHANNELSTATE_READY;
	_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}


uint32 TCD_FTDICHIP::ReleaseChannel()
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	m_tcdData->channelState = CHANNELSTATE_RELEASED;
	delete m_tcdData->pCommunication;
	delete m_tcdData->pConfiguration;
	m_tcdData->pCommunication = NULL;
	m_tcdData->pConfiguration = NULL;
	FT_Close(m_tcdHandle);
	/*m_tcdData->channelState = CHANNEL_RELEASED;*/
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return TCD_OK;
}

uint32 TCD_FTDICHIP::LoadConfigurationFromFile(std::string strPath)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_QUERY))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	if (strPath.empty())
	{
		returnValue = TCD_PARAM_ERROR;
	}

	returnValue = m_tcdData->pConfiguration->LoadConfigurationFromFile(strPath);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::SaveConfigurationToFile(std::string strPath)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_QUERY))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	if (strPath.empty())
	{
		returnValue = TCD_PARAM_ERROR;
	}

	returnValue = m_tcdData->pConfiguration->SaveConfigurationToFile(strPath);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::GetChannelInfo(PTCDChannelInfo pchannelInfo)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_QUERY))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	if (NULL == pchannelInfo)
	{
		returnValue = TCD_PARAM_ERROR;
		goto _end;
	}

	memcpy(pchannelInfo->manufactureID, m_tcdData->channelInfo.manufactureID, sizeof(m_tcdData->channelInfo.manufactureID));
	memcpy(pchannelInfo->manufacture, m_tcdData->channelInfo.manufacture, sizeof(m_tcdData->channelInfo.manufacture));
	memcpy(pchannelInfo->serialNumber, m_tcdData->channelInfo.serialNumber, sizeof(m_tcdData->channelInfo.serialNumber));
	memcpy(pchannelInfo->description, m_tcdData->channelInfo.description, sizeof(m_tcdData->channelInfo.description));
	
	_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::SetChannelInfo(const PTCDChannelInfo pchannelInfo)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_CONFIG))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	//EE must be read first because programData is needed to be an input to program.
	TCDChannelInfo tempChannelInfo;
	FT_PROGRAM_DATA programData;
	FT_STATUS ftStatus;
	programData.Signature1 = 0x00000000;
	programData.Signature2 = 0xffffffff;
	programData.Version = 0x00000005;	// 0x05 means FT232H extensions
	programData.Manufacturer = tempChannelInfo.manufacture;
	programData.ManufacturerId = tempChannelInfo.manufactureID;
	programData.Description = tempChannelInfo.description;
	programData.SerialNumber = tempChannelInfo.serialNumber;

	ftStatus = FT_EE_Read(m_tcdHandle, &programData);
	if (ftStatus != FT_OK)
	{
		returnValue = TCD_FAIL_TO_PROGRAM;
		goto _end;
	}

	ftStatus = FT_EE_ProgramEx(m_tcdHandle, &programData, pchannelInfo->manufacture, pchannelInfo->manufactureID,\
		pchannelInfo->description, pchannelInfo->serialNumber);
	if (ftStatus != FT_OK)
	{
		returnValue = TCD_FAIL_TO_PROGRAM;
		goto _end;
	}
	
	if (0 != RefreshChannelInfo())
	{
		returnValue = TCD_FAIL_TO_READEE;
		goto _end;
	}
	
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

int TCD_FTDICHIP::RefreshChannelInfo()
{
	FT_PROGRAM_DATA ftData;
	FT_STATUS ftStatus;
	ftData.Signature1 = 0x00000000;
	ftData.Signature2 = 0xffffffff;
	ftData.Version = 0x00000005;	// 0x05 means FT232H extensions
	ftData.Manufacturer = m_tcdData->channelInfo.manufacture;
	ftData.ManufacturerId = m_tcdData->channelInfo.manufactureID;
	ftData.Description = m_tcdData->channelInfo.description;
	ftData.SerialNumber = m_tcdData->channelInfo.serialNumber;

	ftStatus = FT_EE_Read(m_tcdHandle, &ftData);
	if (FT_OK != ftStatus)
	{
		return -1;
	}

	std::string strSerialNumber(m_tcdData->channelInfo.serialNumber);
	m_tcdData->pConfiguration->SetSerialNumber(strSerialNumber);
	return 0;
}

uint32 TCD_FTDICHIP::AutoSetChannelSerialNumber(void)
{
	const int len = sizeof(m_tcdData->channelInfo.serialNumber);
	char *str = new char[len];
	if (NULL == str)
	{
		return  TCD_MEMORY_ERROR;
	}

	//Serial number no less than 8
	//len is changed to 8
	srand((unsigned int)time(0));
	for(int i=0;i != 8;++i)
		{
			str[i]='A'+rand()%26;
		}
	str[8] = 0;
	TCDChannelInfo channelInfo;

	if (TCD_OK != GetChannelInfo(&channelInfo))
	{
		return TCD_OTHER_ERROR;
	}
	 
	memcpy(channelInfo.serialNumber, str, sizeof(channelInfo.serialNumber));
	delete str;
	return (SetChannelInfo(&channelInfo));
}

uint32 TCD_FTDICHIP::GetChannelAvailableFormat(puchar formatByte)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_QUERY))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue =m_tcdData->pConfiguration->GetChannelAvailableFormat(formatByte);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::GetChannelFormat(puchar formatByte)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_QUERY))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = m_tcdData->pConfiguration->GetChannelFormat(formatByte);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::SetChannelFormat(uchar formatByte)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_CONFIG))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = m_tcdData->pConfiguration->SetChannelFormat(formatByte);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::GetChannelCommonConfiguration(pTCDChannelCommonConfiguration pcommonConfig)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_QUERY))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = m_tcdData->pConfiguration->GetChannelCommonConfiguration(pcommonConfig);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::GetChannelI2CConfiguration(pTCDChannelI2CConfiguration pI2CConfig)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_QUERY))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}
	returnValue = m_tcdData->pConfiguration->GetChannelI2CConfiguration(pI2CConfig);

_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

//!Get channel UART configuration.
//!@param [out] UARTConfig UART configuration.
//!@return TCD_OK if success.
uint32 TCD_FTDICHIP::GetChannelUARTConfiguration(pTCDChannelUARTConfiguration pUARTConfig)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_QUERY))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = m_tcdData->pConfiguration->GetChannelUARTConfiguration(pUARTConfig);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

//!Get channel common configuration hint.
//!@param [out] commonConfigHint Common configuration hint.
//!@return TCD_OK if success.
uint32 TCD_FTDICHIP::GetChannelCommonConfigurationHint(pTCDChannelCommonConfigHint pcommonConfigHint)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_QUERY))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = m_tcdData->pConfiguration->GetChannelCommonConfigurationHint(pcommonConfigHint);

_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

//!Get channel I2C configuration hint.
//!@param [out] I2CConfigHint I2C configuration hint.
//!@return TCD_OK if success.
uint32 TCD_FTDICHIP::GetChannelI2CConfigurationHint(pTCDChannelI2CConfigHint pI2CConfigHint)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_QUERY))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = m_tcdData->pConfiguration->GetChannelI2CConfigurationHint(pI2CConfigHint);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

//!Get channel UART configuration hint.
//!@param [out] UARTConfigHint UART configuration hint.
//!@return TCD_OK if success.
uint32 TCD_FTDICHIP::GetChannelUARTConfigurationHint(pTCDChannelUARTConfigHint pUARTConfigHint)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_QUERY))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = m_tcdData->pConfiguration->GetChannelUARTConfigurationHint(pUARTConfigHint);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

/*TODO: It's better to use string.
*/
uint32 TCD_FTDICHIP::CheckChannelCommConfiguration(const pTCDChannelCommonConfiguration pcommonConfig, std::string &resultHint)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_CONFIG))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = (m_tcdData->pConfiguration->CheckChannelCommConfiguration(pcommonConfig, resultHint));
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::CheckChannelI2CConfiguration(const pTCDChannelI2CConfiguration pI2CConfig, std::string &resultHint)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_CONFIG))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue =  m_tcdData->pConfiguration->CheckChannelI2CConfiguration(pI2CConfig, resultHint);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::CheckChannelUARTConfiguration(const pTCDChannelUARTConfiguration pUARTConfig, std::string &resultHint)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_CONFIG))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = m_tcdData->pConfiguration->CheckChannelUARTConfiguration(pUARTConfig, resultHint);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::SaveChannelCommonConfiguration(const pTCDChannelCommonConfiguration pcommonConfig)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_CONFIG))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = m_tcdData->pConfiguration->SaveChannelCommonConfiguration(pcommonConfig);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}


uint32 TCD_FTDICHIP::SaveChannelI2CConfiguration(const pTCDChannelI2CConfiguration pI2CConfig)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_CONFIG))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = m_tcdData->pConfiguration->SaveChannelI2CConfiguration(pI2CConfig);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::SaveChannelUARTConfiguration(const pTCDChannelUARTConfiguration pUARTConfig)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_CONFIG))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = m_tcdData->pConfiguration->SaveChannelUARTConfiguration(pUARTConfig);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
	
}

uint32 TCD_FTDICHIP::ChannelTransfer(puchar ptransferBuffer, uint32 bytesToTransfer, puint32 pbytesTransferred)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_COMM))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = m_tcdData->pCommunication->ChannelTransfer(ptransferBuffer, bytesToTransfer, pbytesTransferred);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::ChannelReceive(puchar preceiveBuffer, uint32 bytesToReceive, puint32 pbytesReceived)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_COMM))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue =  m_tcdData->pCommunication->ChannelReceive(preceiveBuffer, bytesToReceive, pbytesReceived);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::GetChannelQueueStatus(puint32 pRxSize, puint32 pTxSize)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_COMM))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = m_tcdData->pCommunication->GetChannelQueueStatus(pRxSize, pTxSize);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::ChannelBulkTransfer(puchar pbulkTransferBuffer, uint32 bytesToTransfer, puint32 pbytesTransferred, uint32 currentBulkIndex, uint32 totalBulkNumber)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_COMM))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = m_tcdData->pCommunication->ChannelBulkTransfer(pbulkTransferBuffer, bytesToTransfer, pbytesTransferred, currentBulkIndex, totalBulkNumber);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::ChannelBulkReceive(puchar pbulkReceiveBuffer, uint32 bytesToReceive, puint32 pbytesReceived, uint32 currentBulkIndex, uint32 totalBulkNumber)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_COMM))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = m_tcdData->pCommunication->ChannelBulkReceive(pbulkReceiveBuffer, bytesToReceive, pbytesReceived, currentBulkIndex, totalBulkNumber);
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

uint32 TCD_FTDICHIP::ChannelBulkClear()
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_COMM))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = (m_tcdData->pCommunication->ReceiveBulkClear());
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}


uint32 TCD_FTDICHIP::SetAddress(uchar addr)
{
	EnterCriticalSection(&m_tcdData->m_channelSLock);
	uint32 returnValue = TCD_OK;
	if (!CheckStateOperation(m_tcdData->channelState, OPERATE_COMM))
	{
		returnValue = TCD_OPERATION_NOT_ALLOWD;
		goto _end;
	}

	returnValue = (m_tcdData->pCommunication->SetAddress(addr));
_end:
	LeaveCriticalSection(&m_tcdData->m_channelSLock);
	return returnValue;
}

static bool CheckStateOperation(int channelState, int operation)
{
	//State -- Operation Corresponding Table
	//|CHANNELSTATE_NOT_INITIALED|   OPERATE_INIT							|
	//|													_|____________________________________
	//|													|	OPERATE_RELEASE					|
	//|___________________________________|____________________________________|
	//|CHANNELSTATE_READY			|	All operation are allowed			|
	//|___________________________________|____________________________________|
	//|CHANNELSTATE_RELEASED		|	None is allowed						|
	//|________________________________________________________________________|
	switch (channelState)
	{
		//None operation is not allowed, exclude init
	case CHANNELSTATE_NOT_INITIALED:
		if (OPERATE_INIT != operation && OPERATE_RELEASE != operation)
		{
			return false;
		}
		return true;
	case CHANNELSTATE_READY:
		return true;
	//case CHANNELSTATE_CONFIG:
	//	return false;
	//case CHANNELSTATE_COMM:
	//	if (OPERATE_QUERY != operation)
	//	{
	//		return false;
	//	}
	//	return true;
	case CHANNELSTATE_RELEASED:
		return false;
	default:
			return false;
	}
}	