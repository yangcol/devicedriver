#include "TCD_FTDICHIP_ChannelCommunication.h"
#pragma comment(lib, "ftd2xx.lib")
#include <Windows.h>
#include "ftd2xx.h"
#include "libMPSSE_i2c.h"

using namespace TCDNameSpace::TCDCommunication;
using TCDNameSpace::TCDConfiguration::TCD_FORMAT_I2C;
using TCDNameSpace::TCDConfiguration::TCD_FORMAT_UART;

namespace {
	enum 
	{
		I2C_DEVICE_ADDRESS_EEPROM		= 0x60, //0x57
		I2C_DEVICE_BUFFER_SIZE		= 256,
		I2C_WRITE_COMPLETION_RETRY		= 10,
		START_ADDRESS_EEPROM 	= 0x00 ,/*read/write start address inside the EEPROM*/
		END_ADDRESS_EEPROM		= 0x10,
		RETRY_COUNT_EEPROM		= 10,	/* number of retries if read/write fails */
		CHANNEL_TO_OPEN			= 0,	/*0 for first available channel, 1 for next... */
		DATA_OFFSET				= 2
	};
	const uint32 default_I2C_Option = I2C_TRANSFER_OPTIONS_START_BIT| I2C_TRANSFER_OPTIONS_BREAK_ON_NACK ;
	const uint32 default_addr = 0x60;
	//Communication part
	HMODULE h_libMPSSE;
	typedef FT_STATUS (*pfunc_I2C_DeviceRead)(FT_HANDLE handle,uint32 \
		deviceAddress,uint32 sizeToTransfer, uint8 *buffer, uint32 \
		*sizeTransfered, uint32 options);
	pfunc_I2C_DeviceRead p_I2C_DeviceRead;
	typedef FT_STATUS (*pfunc_I2C_DeviceWrite)(FT_HANDLE handle, uint32 \
		deviceAddress,uint32 sizeToTransfer, uint8 *buffer, uint32 \
		*sizeTransfered, uint32 options);
	pfunc_I2C_DeviceWrite p_I2C_DeviceWrite;

	typedef FT_STATUS (*pfunc_I2C_InitChannel)(FT_HANDLE handle, ChannelConfig \
		*config);
	pfunc_I2C_InitChannel p_I2C_InitChannel;
};

static bool CheckBulkVector(std::vector<bool> bulkVector, uint32 currentBulkIndex, uint32 totalBulkNumber);
static bool IsBulkFinished(std::vector<bool> bulkVector);

static inline int CheckError(PVOID expression)
{
	if (NULL == expression)
	{
		return -1;
	}

	return 0;
}

TCD_FTDICHIP_ChannelCommunication::TCD_FTDICHIP_ChannelCommunication(TCDHandle tcdHandle)
{
	m_tcdHandle = tcdHandle;
	m_address = 0xC0;
}

int TCD_FTDICHIP_ChannelCommunication::SetFormat(uchar format)
{
	m_format = format;
	return 0;
}

TCD_FTDICHIP_ChannelCommunication::~TCD_FTDICHIP_ChannelCommunication(void)
{
	FreeLibrary(h_libMPSSE);
}

uint32 TCD_FTDICHIP_ChannelCommunication::Initial()
{
	h_libMPSSE = LoadLibrary("libMPSSE.dll");
	if (NULL == h_libMPSSE)
	{
		return TCD_OTHER_ERROR;
	}

	p_I2C_DeviceRead = (pfunc_I2C_DeviceRead)GetProcAddress(h_libMPSSE\
		, "I2C_DeviceRead");

	p_I2C_DeviceWrite = (pfunc_I2C_DeviceWrite)GetProcAddress(h_libMPSSE\
		, "I2C_DeviceWrite");

	p_I2C_InitChannel = (pfunc_I2C_InitChannel)GetProcAddress(h_libMPSSE\
		, "I2C_InitChannel");

	if (CheckError(p_I2C_DeviceRead)||CheckError(p_I2C_DeviceWrite) || CheckError(p_I2C_InitChannel))
	{
		return TCD_FAIL_INITIAL;
	}
	
	if (0 != Reset())
	{
		return TCD_FAIL_INITIAL;
	}

	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelCommunication::Reset()
{
	//Purge chip send and receivebuffer
	ReceiveBulkClear();
	TransferBulkClear();
	FT_STATUS ft = FT_OK;
	ft = FT_Purge(m_tcdHandle, FT_PURGE_RX | FT_PURGE_TX);
	if (FT_OK != ft)
	{
		return -1;
	}

	return TCD_OK;
}

int TCD_FTDICHIP_ChannelCommunication::SetAddress(uchar addr)
{
	if (!m_receiveBulkState.empty() || !m_transferBulkState.empty())
	{
		return -1;
	}

	m_address = addr;
	return 0;
}

uint32 TCD_FTDICHIP_ChannelCommunication::ChannelTransfer(puchar ptransferBuffer, uint32 bytesToTransfer, puint32 pbytesTransferred)
{
	//
	FT_STATUS ftStatus = FT_OK;

	if (TCD_FORMAT_I2C == m_format)
	{
		//Check if send is successful;
		bool writeComplete = false;
		int retryCount = 0;
		while(!writeComplete && retryCount < I2C_WRITE_COMPLETION_RETRY)
		{
			ftStatus = p_I2C_DeviceWrite(m_tcdHandle, m_address, bytesToTransfer /*+ 1*/, ptransferBuffer, pbytesTransferred, default_I2C_Option);
			retryCount++;
			writeComplete = (bytesToTransfer == *pbytesTransferred && ftStatus == FT_OK)? true:false;
		}
	}

	if (TCD_FORMAT_UART == m_format)
	{
		ftStatus = FT_Write(m_tcdHandle, ptransferBuffer, bytesToTransfer, pbytesTransferred);
	}
	//delete buffer;
	if (FT_OK != ftStatus)
	{
		return TCD_IO_ERROR; 
	}

	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelCommunication::ChannelReceive(puchar preceiveBuffer, uint32 bytesToReceive, puint32 pbytesReceived)
{
	FT_STATUS ftStatus;

	if (TCD_FORMAT_I2C == m_format)
	{
		ftStatus = p_I2C_DeviceRead(m_tcdHandle, m_address, bytesToReceive, preceiveBuffer, pbytesReceived, default_I2C_Option);
	}

	if (TCD_FORMAT_UART == m_format)
	{
		ftStatus = FT_Read(m_tcdHandle, preceiveBuffer, bytesToReceive, pbytesReceived);
	}

	if (FT_OK != ftStatus)
	{
		return TCD_IO_ERROR; 
	}

	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelCommunication::GetChannelQueueStatus(puint32 pRxSize, puint32 pTxSize)
{
	FT_STATUS ftStatus = FT_GetQueueStatus(m_tcdHandle, pRxSize);
	if (FT_OK != ftStatus)
	{
		return TCD_OTHER_ERROR;
	}

	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelCommunication::ChannelBulkTransfer(puchar pbulkTransferBuffer, uint32 bytesToTransfer, puint32 pbytesTransferred, uint32 currentBulkIndex, uint32 totalBulkNumber)
{
	if (!CheckBulkVector(m_transferBulkState, currentBulkIndex, totalBulkNumber))
	{
		return TCD_BULK_ERROR;
	}

	if (TCD_OK !=ChannelTransfer(pbulkTransferBuffer, bytesToTransfer, pbytesTransferred))
	{
		return TCD_IO_ERROR;
	}

	m_transferBulkState.at(currentBulkIndex) = true;
	if (IsBulkFinished(m_transferBulkState))
	{
		TransferBulkClear();
	}
	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelCommunication::ChannelBulkReceive(puchar pbulkReceiveBuffer, uint32 bytesToReceive, puint32 pbytesReceived, uint32 currentBulkIndex, uint32 totalBulkNumber)
{
	if (!CheckBulkVector(m_receiveBulkState, currentBulkIndex, totalBulkNumber))
	{
		return TCD_BULK_ERROR;
	}

	if (TCD_OK == ChannelReceive(pbulkReceiveBuffer, bytesToReceive, pbytesReceived))
	{
		return TCD_IO_ERROR;
	}

	m_receiveBulkState.at(currentBulkIndex) = true;
	if (IsBulkFinished(m_receiveBulkState))
	{
		ReceiveBulkClear();
	}
	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelCommunication::ReceiveBulkClear()
{
	m_receiveBulkState.clear();
	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelCommunication::TransferBulkClear()
{
	m_transferBulkState.clear();
	return TCD_OK;
}

static bool CheckBulkVector(std::vector<bool> bulkVector, uint32 currentBulkIndex, uint32 totalBulkNumber)
{
	if (totalBulkNumber != bulkVector.size())
	{
		return false;
	}

	if (true == bulkVector.at(currentBulkIndex))
	{
		//this bulk already transferred/received
		return false;
	}
	
	return true;
}

//Check if to clear datavector.
static bool IsBulkFinished(std::vector<bool> bulkVector)
{
	if (bulkVector.empty())
	{
		return false;
	}

	std::vector<bool>::iterator iterBulkVector = bulkVector.begin();
	for (; iterBulkVector != bulkVector.end(); ++iterBulkVector)
	{
		if (false == *iterBulkVector)
		{
			return false;
		}
	}

	return true;
}