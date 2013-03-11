//!@file TCD_FTDICHIP.h
//!@author QingYang
//!@brief This file identifies class TCD_FTDICHIP.
//!@version 1.0
//!@date 2012/9/18
//!<<pre>qingy@fortemedia.com</pre>
//!<<b>All rights reserved.</b>
#pragma once
#include "TuningChannelDriver.h"

class TCD_FTDICHIP : public TuningChannelDriver
{
public:
	//Build TCD_FTDICHIP instance.
	TCD_FTDICHIP(TCDHandle tcdHandle);
	
	//Standard constructor and SetSourceHandle to build an TCD_FTDICHIP instance.
	//TCD_FTDICHIP();
	//void SetSourceHandle(TCDHandle tcdHandle);
	
 ~TCD_FTDICHIP(void);
public:
	//!@name Initial & Reset
	//!@{

	//!Initial TuningServerDriver. TuningServerDriver must be initialed before used.
	//!@param None
	//!@return TCD_OK if success.
	uint32 InitialChannel();

	//!Reset TuningServerDriver.
	//!@param None
	//!@return TCD_OK if success.
	//!@remark RestChannel 
	uint32 ResetChannel();

	//!Release TuningServerDriver.
	//!@param None
	//!@return TCD_OK if success.
	//!@remark ReleaseChannel is called when this channel is no longer used.
	uint32 ReleaseChannel();
//
	//!@}

	//!@name Channel Information
	//!@{

	//! Get channel information into struct.
	//!@param [in] pchannelInfo Channel info struct.
	//!@return TCD_OK if success.


	uint32 LoadConfigurationFromFile(std::string strPath);
	uint32 SaveConfigurationToFile(std::string strPath);

	uint32 GetChannelInfo(PTCDChannelInfo pchannelInfo);

	//!Set channel information with struct.
	//!@param [in] pchannelInfo Channel info struct.
	//!@return TCD_OK if success.
	uint32 SetChannelInfo(const PTCDChannelInfo pchannelInfo);

	//!Auto set channel a unique serial number.
	//!@param None
	//!@return TCD_OK If success.
	//TODO Check if this API is useful.
	uint32 AutoSetChannelSerialNumber(void);
	//!@}

	//!@name Channel Configuration
	//!@{

	//!Get channel available format.
	//!@param [out] formatByte Byte to store format result.
	//!@return TCD_OK if success.
	uint32 GetChannelAvailableFormat(puchar pformatByte);

	//!Set channel format.
	//!@param [in] formatByte Byte to store format result.
	//!@return TCD_OK if success.
	uint32 SetChannelFormat(uchar formatByte);

	//!Get channel's current format.
	//!@param [out] formatByte Byte to store format result.
	//!@return TCD_OK if success.
	uint32 GetChannelFormat(puchar formatByte);

	//!Get channel common configuration.
	//!@param [out] commonConfig Common configuration.
	//!@return TCD_OK if success.
	uint32 GetChannelCommonConfiguration(pTCDChannelCommonConfiguration pcommonConfig);

	//!Get channel I2C configuration.
	//!@param [out] I2CConfig I2C configuration.
	//!@return TCD_OK if success.
	uint32 GetChannelI2CConfiguration(pTCDChannelI2CConfiguration pI2CConfig);

	//!Get channel UART configuration.
	//!@param [out] UARTConfig UART configuration.
	//!@return TCD_OK if success.
	uint32 GetChannelUARTConfiguration(pTCDChannelUARTConfiguration pUARTConfig);

	//!Get channel common configuration hint.
	//!@param [out] commonConfigHint Common configuration hint.
	//!@return TCD_OK if success.
	uint32 GetChannelCommonConfigurationHint(pTCDChannelCommonConfigHint pcommonConfigHint);

	//!Get channel I2C configuration hint.
	//!@param [out] I2CConfigHint I2C configuration hint.
	//!@return TCD_OK if success.
	uint32 GetChannelI2CConfigurationHint(pTCDChannelI2CConfigHint pI2CConfigHint);

	//!Get channel UART configuration hint.
	//!@param [out] UARTConfigHint UART configuration hint.
	//!@return TCD_OK if success.
	uint32 GetChannelUARTConfigurationHint(pTCDChannelUARTConfigHint pUARTConfigHint);

	//!Check channel common configuration.
	//!@param [in] commonConfig Common configuration.
	//!@param [out] resultHint Hint string to store result of check result.
	//!@param [in] resultHintLength Length of result hint in bytes.
	//!@return TCD_OK if success.
	//!@remarks If return value is not TCD_OK, resultHint will get a string
	//! to describe which parameter of TCDChannelCommonConfigHint is not valid.
	//! Common configuration will stay until SaveChannelCommonConfiguration is called.
	uint32 CheckChannelCommConfiguration(const pTCDChannelCommonConfiguration pcommonConfig, std::string &resultHint);

	//!Check channel I2C configuration.
	//!@param [in] I2CConfig I2C configuration.
	//!@param [out] resultHint Hint string to store result of check result.
	//!@param [in] resultHintLength Length of result hint in bytes.
	//!@return TCD_OK if success.
	//!@remarks If return value is not TCD_OK, resultHint will get a string
	//! to describe which parameter of TCDChannelI2CConfigHint is not valid.
	//! I2C configuration will stay until SaveChannelI2CConfiguration is called.
	uint32 CheckChannelI2CConfiguration(const pTCDChannelI2CConfiguration pI2CConfig, std::string &resultHint);

	//!Check channel UART configuration.
	//!@param [in] UARTConfig UART configuration.
	//!@param [out] resultHint Hint string to store result of check result.
	//!@param [in] resultHintLength Length of result hint in bytes.
	//!@return TCD_OK if success.
	//!@remarks If return value is not TCD_OK, resultHint will get a string
	//! to describe which parameter of TCDChannelUARTConfigHint is not valid.
	//! UART configuration will stay until SaveChannelUARTConfiguration is called.
	uint32 CheckChannelUARTConfiguration(const pTCDChannelUARTConfiguration pUARTConfig, std::string &resultHint);

	//!Save channel common configuration.
	//!@param [in] commonConfig Common configuration.
	//!@return TCD_OK if configuration is valid.
	uint32 SaveChannelCommonConfiguration(const pTCDChannelCommonConfiguration pcommonConfig);

	//!Save channel I2C configuration.
	//!@param [in] I2CConfig I2C configuration.
	//!@return TCD_OK if success.
	uint32 SaveChannelI2CConfiguration(const pTCDChannelI2CConfiguration pI2CConfig);

	//!Save channel UART configuration.
	//!@param [in] UARTConfig UART configuration.
	//!@return TCD_OK if success.
	uint32 SaveChannelUARTConfiguration(const pTCDChannelUARTConfiguration pUARTConfig);
	//!@}

	//!@name Channel Communication
	//!@{

	//!Use channel to transfer data.
	//!@param [in] transferBuffer Buffer to transfer.
	//!@param [in] bytesToTransfer Transfer size in bytes.
	//!@param [out] bytesTransferred Transferred size in bytes.
	//!@return TCD_OK if success.
	uint32 ChannelTransfer(const puchar ptransferBuffer, uint32 bytesToTransfer, puint32 pbytesTransferred);

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
	uint32 ChannelBulkTransfer(const puchar pbulkTransferBuffer, uint32 bytesToTransfer, puint32 pbytesTransferred, uint32 currentBulkIndex, uint32 totalBulkNumber);

	//!Bulk receive is used for receive several block data.
	//!@param [in] bulkReceiveBuffer Bulk receive buffer.
	//!@param [in] bytesToReceive Size to receive in bytes.
	//!@param [out] bytesReceived Size receive in bytes.
	//!@param [in] currentBulkIndex Current bulk index, starts from 0.
	//!@param [in] totalBulkNumber Total bulk number.
	uint32 ChannelBulkReceive(puchar pbulkReceiveBuffer, uint32 bytesToReceive, puint32 pbytesReceived, uint32 currentBulkIndex, uint32 totalBulkNumber);
	//!@}

	//!Clear bulk state.
	//!@param N/A
	//!@return TCD_OK if success.
	uint32 ChannelBulkClear();	

	uint32 SetAddress(uchar addr);
protected:

	//!Prepare channel for communication.
	uint32 ChannelCommPrepare();

	//!Clear communication state. 
	uint32 ChannelCommPurge();
	uint32 ChannelCommBreakOn();
	uint32 ChannelCommBreakOff();

	//!Finish current communication task.
	uint32 ChannelCommRelease();
private:
	struct TCDDataStruct;
	TCDDataStruct* m_tcdData;			//<Store data used by Tuning Channel Driver.
	TCDHandle m_tcdHandle;				///<TCDHandle to access channel.
	inline bool IsInitialized();

	//!Refresh channel info from EEPROM
	int RefreshChannelInfo();
};

