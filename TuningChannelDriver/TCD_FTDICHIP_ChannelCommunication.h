//!@file TCD_FTDICHIP_ChannelCommunication.h
//!@author QingYang
//!@brief This file identifies class TCD_FTDICHIP_ChannelCommunication.
//!@version 1.0
//!@date 2012/9/18
//!<<pre>qingy@fortemedia.com</pre>
//!<<b>All rights reserved.</b>
#pragma once
#include "TCDDefine.h"
#include <vector>

using TCDNameSpace::TCDConfiguration::pTCDChannelCommonConfiguration;
using TCDNameSpace::TCDConfiguration::pTCDChannelI2CConfiguration;
using TCDNameSpace::TCDConfiguration::pTCDChannelUARTConfiguration;


class TCD_FTDICHIP_ChannelCommunication
{
public:
	TCD_FTDICHIP_ChannelCommunication(TCDHandle tcdHandle);
	~TCD_FTDICHIP_ChannelCommunication(void);

	uint32 Initial();
	//!Use channel to transfer data.
	//!@param [in] transferBuffer Buffer to transfer.
	//!@param [in] bytesToTransfer Transfer size in bytes.
	//!@param [out] bytesTransferred Transferred size in bytes.
	//!@return TCD_OK if success.
	uint32 ChannelTransfer(puchar ptransferBuffer, uint32 bytesToTransfer, puint32 pbytesTransferred);

	//!Use channel to transfer data.
	//!@param [in] receiveBuffer Buffer to receive.
	//!@param [in] bytesToReceive Receive size in bytes.
	//!@param [out] bytesReceived Received size in bytes.
	//!@return TCD_OK if success.
	uint32 ChannelReceive(puchar preceiveBuffer, uint32 bytesToReceive, puint32 pbytesReceived);

	//!Get channel communication status.
	//!@param [out] RxSize Receive queue length in bytes.
	//!@param [out] TxSize Transfer queue length in bytes. 
	uint32 GetChannelQueueStatus(puint32 pRxSize, puint32 pTxSize);

	//!Bulk transfer is used for transfer several block data.
	//!@param [in] bulkTransferBuffer Bulk transfer buffer.
	//!@param [in] bytesToTransfer Size to transfer in bytes.
	//!@param [out] bytesTransferred Size transferred in bytes.
	//!@param [in] currentBulkIndex Current bulk index, starts from 0.
	//!@param [in] totalBulkNumber Total bulk number.
	uint32 ChannelBulkTransfer(puchar pbulkTransferBuffer, uint32 bytesToTransfer, puint32 pbytesTransferred, uint32 currentBulkIndex, uint32 totalBulkNumber);

	//!Bulk receive is used for receive several block data.
	//!@param [in] bulkReceiveBuffer Bulk receive buffer.
	//!@param [in] bytesToReceive Size to receive in bytes.
	//!@param [out] bytesReceived Size receive in bytes.
	//!@param [in] currentBulkIndex Current bulk index, starts from 0.
	//!@param [in] totalBulkNumber Total bulk number.
	uint32 ChannelBulkReceive(puchar pbulkReceiveBuffer, uint32 bytesToReceive, puint32 pbytesReceived, uint32 currentBulkIndex, uint32 totalBulkNumber);

	uint32 ReceiveBulkClear();	
	uint32 TransferBulkClear();

	uint32 SetSalveAddress(uchar addr);
	uint32 SetFormat(uchar format);
	uint32 SetSendOption(uchar sendOption);
	uint32 SetReceiveOption(uchar receiveOption);

	//!Clear bulk state.
	//!@param N/A
	//!@return TCD_OK if success.
	uint32 Reset();

	
protected:

	//!Prepare channel for communication.
	uint32 ChannelCommPrepare();
	
	//!Clear communication state. 
	uint32 ChannelCommPurge();
	uint32 ChannelCommBreakOn();
	uint32 ChannelCommBreakOff();

	//!Finish current communication task.
	uint32 ChannelCommRelease();

protected:
	//TCD_FTDICHIP_ChannelCommunication(void);
private:
	TCDHandle m_tcdHandle;
	uchar m_format;
	uchar m_address;
	std::vector<bool> m_transferBulkState;
	std::vector<bool> m_receiveBulkState;
	enum 
	{
		I2C_WRITE_COMPLETION_RETRY = 10
	};

	uchar m_sendOption;
	uchar m_receiveOption;
	//TCDConfiguration::TCDChannelI2CConfiguration m_i2cConfiguration;
	//TCDConfiguration::TCDChannelUARTConfiguration m_uartConfiguration;
	//TCDConfiguration::pTCDChannelCommonConfiguration m_comConfiguration;
};

