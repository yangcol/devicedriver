#pragma once
#include "TCDDefine.h"
#include <string>

using namespace TCDNameSpace::TCDConfiguration;

class TCDCommunication;

class TCD_FTDICHIP_ChannelConfiguration
{
public:
	TCD_FTDICHIP_ChannelConfiguration(TCDHandle tcdHandle);
	~TCD_FTDICHIP_ChannelConfiguration(void);

	uint32 Initial();

	uint32 GetChannelAvailableFormat(puchar pformatByte);

	uint32 SetChannelFormat(uchar formatByte);

	uint32 GetChannelFormat(puchar formatByte);

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

	uint32 Reset();

	int SetSerialNumber(std::string strSerialNumber);

	//Load configuration from file
	int LoadConfigurationFromFile(std::string strPath);

	int SaveConfigurationToFile(std::string strPath);

	int GetAddress(puchar paddr);
protected:
	//TCD_FTDICHIP_ChannelConfiguration(void);
private:

	TCDHandle m_tcdHandle;				///<TCDHandle to access channel.
	
	int UpdateConfigurationFile(int configType);
	int LoadDefaultConfiguration();


	int SetChannelCommonConfiguration(const pTCDChannelCommonConfiguration pcommonConfig);
	int SetChannelI2CConfiguration(const pTCDChannelI2CConfiguration pI2CConfig);
	int SetChannelUARTConfiguration(const pTCDChannelUARTConfiguration pUARTConfig);

	uchar m_format;						//<Indicate current format.
	std::string m_serialNumber;
	TCDChannelCommonConfiguration m_channelCommConfig;
	TCDChannelUARTConfiguration m_channelUARTConfig;
	TCDChannelI2CConfiguration m_channelI2CConfig;
	uchar m_address;
};

