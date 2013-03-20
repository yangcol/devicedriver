//!@file TuningServerDriver.h
//!@author QingYang
//!@brief This file identifies class TningServerDriver.
//!@version 1.0
//!@date 2012/9/18
//!<<pre>qingy@fortemedia.com</pre>
//!<<b>All rights reserved.</b>
#pragma once

#include <string>
#include "TCDDefine.h"

using namespace TCDNameSpace;
using namespace TCDNameSpace::TCDConfiguration;
//!@brief This class is not finished yet.
class TuningChannelDriver
{
public:
	//!@name Initial & Reset
	//!@{

	//!Initial TuningServerDriver. TuningServerDriver must be initialed before used.
	//!@param None
	//!@return TCD_OK if success.
	virtual uint32 InitialChannel() = 0;

	//!Reset TuningServerDriver.
	//!@param None
	//!@return TCD_OK if success.
	//!@remark RestChannel 
	virtual uint32 ResetChannel() = 0;

	//!Release TuningServerDriver.
	//!@param None
	//!@return TCD_OK if success.
	//!@remark ReleaseChannel is called when this channel is no longer used.
	virtual uint32 ReleaseChannel() = 0;

	//!@}

	//!@name Channel Information
	//!@{

	//! Get channel information into struct.
	//!@param [in] pchannelInfo Channel info struct.
	//!@return TCD_OK if success.

	virtual uint32 LoadConfigurationFromFile(std::string strPath) = 0;

	virtual uint32 SaveConfigurationToFile(std::string strPath) = 0;

	virtual uint32 GetChannelInfo(pTCDChannelInfo pchannelInfo) = 0;

	//!Set channel information with struct.
	//!@param [in] pchannelInfo Channel info struct.
	//!@return TCD_OK if success.
	virtual uint32 SetChannelInfo(const pTCDChannelInfo pchannelInfo) = 0;

	//!Auto set channel a unique serial number.
	//!@param None
	//!@return TCD_OK If success.
	//TODO Check if this API is useful.
	virtual uint32 AutoSetChannelSerialNumber(void) = 0;
	//!@}

	//!@name Channel Configuration
	//!@{

	//!Get channel available format.
	//!@param [out] formatByte Byte to store format result.
	//!@return TCD_OK if success.
	virtual uint32 GetChannelAvailableFormat(puchar formatByte) = 0;
	
	//!Set channel format.
	//!@param [in] formatByte Byte to store format result.
	//!@return TCD_OK if success.
	virtual uint32 SetChannelFormat(uchar formatByte) = 0;

	//!Get channel's current format.
	//!@param [out] formatByte Byte to store format result.
	//!@return TCD_OK if success.
	virtual uint32 GetChannelFormat(puchar formatByte) = 0;

	//!Get channel common configuration.
	//!@param [out] commonConfig Common configuration.
	//!@return TCD_OK if success.
	virtual uint32 GetChannelCommonConfiguration(pTCDChannelCommonConfiguration commonConfig) = 0;

	//!Get channel I2C configuration.
	//!@param [out] I2CConfig I2C configuration.
	//!@return TCD_OK if success.
	virtual uint32 GetChannelI2CConfiguration(pTCDChannelI2CConfiguration I2CConfig) = 0;

	//!Get channel UART configuration.
	//!@param [out] UARTConfig UART configuration.
	//!@return TCD_OK if success.
	virtual uint32 GetChannelUARTConfiguration(pTCDChannelUARTConfiguration UARTConfig) = 0;

	//!Get channel common configuration hint.
	//!@param [out] commonConfigHint Common configuration hint.
	//!@return TCD_OK if success.
	virtual uint32 GetChannelCommonConfigurationHint(pTCDChannelCommonConfigHint commonConfigHint) = 0;

	//!Get channel I2C configuration hint.
	//!@param [out] I2CConfigHint I2C configuration hint.
	//!@return TCD_OK if success.
	virtual uint32 GetChannelI2CConfigurationHint(pTCDChannelI2CConfigHint I2CConfigHint) = 0;

	//!Get channel UART configuration hint.
	//!@param [out] UARTConfigHint UART configuration hint.
	//!@return TCD_OK if success.
	virtual uint32 GetChannelUARTConfigurationHint(pTCDChannelUARTConfigHint UARTConfigHint) = 0;

	//!Check channel common configuration.
	//!@param [in] commonConfig Common configuration.
	//!@param [out] resultHint Hint string to store result of check result.
	//!@param [in] resultHintLength Length of result hint in bytes.
	//!@return TCD_OK if success.
	//!@remarks If return value is not TCD_OK, resultHint will get a string
	//! to describe which parameter of TCDChannelCommonConfigHint is not valid.
	//! Common configuration will stay until SaveChannelCommonConfiguration is called.
	virtual uint32 CheckChannelCommConfiguration(const pTCDChannelCommonConfiguration pcommonConfig, std::string &resultHint) = 0;

	//!Check channel I2C configuration.
	//!@param [in] I2CConfig I2C configuration.
	//!@param [out] resultHint Hint string to store result of check result.
	//!@param [in] resultHintLength Length of result hint in bytes.
	//!@return TCD_OK if success.
	//!@remarks If return value is not TCD_OK, resultHint will get a string
	//! to describe which parameter of TCDChannelI2CConfigHint is not valid.
	//! I2C configuration will stay until SaveChannelI2CConfiguration is called.
	virtual uint32 CheckChannelI2CConfiguration(const pTCDChannelI2CConfiguration pI2CConfig, std::string &resultHint) = 0;

	//!Check channel UART configuration.
	//!@param [in] UARTConfig UART configuration.
	//!@param [out] resultHint Hint string to store result of check result.
	//!@param [in] resultHintLength Length of result hint in bytes.
	//!@return TCD_OK if success.
	//!@remarks If return value is not TCD_OK, resultHint will get a string
	//! to describe which parameter of TCDChannelUARTConfigHint is not valid.
	//! UART configuration will stay until SaveChannelUARTConfiguration is called.
	virtual uint32 CheckChannelUARTConfiguration(const pTCDChannelUARTConfiguration pUARTConfig, std::string &resultHint) = 0;
	
	//!Save channel common configuration.
	//!@param [in] commonConfig Common configuration.
	//!@return TCD_OK if configuration is valid.
	virtual uint32 SaveChannelCommonConfiguration(const pTCDChannelCommonConfiguration pcommonConfig) = 0;

	//!Save channel I2C configuration.
	//!@param [in] I2CConfig I2C configuration.
	//!@return TCD_OK if success.
	virtual uint32 SaveChannelI2CConfiguration(const pTCDChannelI2CConfiguration pI2CConfig) = 0;
	
	//!Save channel UART configuration.
	//!@param [in] UARTConfig UART configuration.
	//!@return TCD_OK if success.
	virtual uint32 SaveChannelUARTConfiguration(const pTCDChannelUARTConfiguration pUARTConfig) = 0;
	//!@}

	//!@name Channel Communication
	//!@{

	//!Use channel to transfer data.
	//!@param [in] transferBuffer Buffer to transfer.
	//!@param [in] bytesToTransfer Transfer size in bytes.
	//!@param [out] bytesTransferred Transferred size in bytes.
	//!@return TCD_OK if success.
	virtual uint32 ChannelTransfer(const puchar transferBuffer, uint32 bytesToTransfer, puint32 bytesTransferred) = 0;

	//!Use channel to transfer data.
	//!@param [in] receiveBuffer Buffer to receive.
	//!@param [in] bytesToReceive Receive size in bytes.
	//!@param [out] bytesReceived Received size in bytes.
	//!@return TCD_OK if success.
	virtual uint32 ChannelReceive(puchar receiveBuffer, uint32 bytesToReceive, puint32 bytesReceived) = 0;

	//!Get channel communication status.
	//!@param [out] RxSize Receive queue length in bytes.
	//!@param [out] TxSize Transfer queue length in bytes. 
	//!@return TCD_OK if success.
	virtual uint32 GetChannelQueueStatus(puint32 RxSize, puint32 TxSize) = 0;

	//!Bulk transfer is used for transfer several block data.
	//!@param [in] bulkTransferBuffer Bulk transfer buffer.
	//!@param [in] bytesToTransfer Size to transfer in bytes.
	//!@param [out] bytesTransferred Size transferred in bytes.
	//!@param [in] currentBulkIndex Current bulk index, starts from 0.
	//!@param [in] totalBulkNumber Total bulk number.
	//!@return TCD_OK if success.
	//!@remark Every time, 
	virtual uint32 ChannelBulkTransfer(const puchar bulkTransferBuffer, uint32 bytesToTransfer, puint32 bytesTransferred, uint32 currentBulkIndex, uint32 totalBulkNumber) = 0;
	
	//!Bulk receive is used for receive several block data.
	//!@param [in] bulkReceiveBuffer Bulk receive buffer.
	//!@param [in] bytesToReceive Size to receive in bytes.
	//!@param [out] bytesReceived Size receive in bytes.
	//!@param [in] currentBulkIndex Current bulk index, starts from 0.
	//!@param [in] totalBulkNumber Total bulk number.
	//!@return TCD_OK if success.
	virtual uint32 ChannelBulkReceive(puchar bulkReceiveBuffer, uint32 bytesToReceive, puint32 bytesReceived, uint32 currentBulkIndex, uint32 totalBulkNumber) = 0;
	
	//!Clear bulk state.
	//!@param N/A
	//!@return TCD_OK if success.
	virtual uint32 ChannelBulkClear() = 0;	

	virtual uint32 SetAddress(uchar addr) = 0;
	//!@}

};

