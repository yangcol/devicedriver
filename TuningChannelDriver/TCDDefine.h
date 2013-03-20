//@file TCDDefine.h
//@author QingYang
//@brief This file describes defination used by Tuning Server Driver
//@version 1.0
// @date 2012/9/18
//!<<pre>qingy@fortemedia.com</pre>
//!<<b>All rights reserved.</b> 
#pragma once

#if defined(__WINDOWS_)

#else
#if defined(linux)

#endif
#endif

#define MAX_PATH_DEPTH 1024
typedef void* TCDHandle;
typedef unsigned char uchar;
typedef uchar* puchar;
typedef unsigned long uint32;
typedef uint32* puint32;
typedef float float32;

enum
{
	TCD_OK = 0,
	TCD_NOT_INITIALED = -1,
	TCD_FAIL_INITIAL = -2,
	TCD_HANDLE_NOT_VALID = -3,
	TCD_ALREADY_IN_USE = -4,
	TCD_NOT_OPENED = -5,
	TCD_IO_ERROR = -6,
	TCD_PARAM_ERROR = -7,
	TCD_MEMORY_ERROR = -8,
	TCD_FAIL_TO_PROGRAM = -9,
	TCD_FAIL_TO_READEE = -10,
	TCD_API_SEQUENCE_ERROR = -11,
	TCD_FORMAT_NOT_SUPPORTED = -12,
	TCD_BULK_ERROR = -13,
	TCD_OPERATION_NOT_ALLOWD = -14,
	TCD_FILE_ERROR = -15,
	TCD_OTHER_ERROR = -16
};

namespace TCDNameSpace
{
	//!Struct TCDChannelInfo stores channel information of TCD.
	struct TCDChannelInfo
	{
		char manufactureID[16];	///<Manufacture ID.
		char manufacture[32];		///<Manufacture.
		char description[64];			///<Description.
		char serialNumber[16];		///<Serial number.
	};

	namespace TCDConfiguration
	{
		//!Struct TCDChannelCommonConfigurtion stores common configuration of TCD.
		struct TCDChannelCommonConfiguration
		{
			uint32 USBBuffer;					///<USB buffer size default to be 4096 (bytes), it must be a multiple of 64 bytes between 64 bytes and 64k bytes.
			uint32 readtimeouts;				///<Read time outs in milliseconds.
			uint32 writetimeouts;			///<Write time outs in milliseconds.
			uint32 retrycount;					///<Retry count. Default to be 50.
			uchar latencytime;				///<Latency time of flush remaining data from the receive buffer. Default to be 16. Range from 2 to 255 with interval 1.
		};

		//!Struct TCDChannelI2CConfiguration stores I2C configuration of TCD.
		struct TCDChannelI2CConfiguration
		{
			uint32 clockrate;					///<clockrate corresponding which chosen from: StandardMode = 100000, \
			//StandardMode3P = 133333, FastMode = 400000, FastModePlus = 100000, \
			//HighSpeedMode = 340000. Default to be StandardMode. 
			bool threephaseclock;			///<Three phase clock.
			//uchar slaveaddress;
			uchar sendOption;				//TODO
			uchar receiveOption;				//TODO
		};

		//!Struct TCDChannelUARTConfigurtion stores UART configuration of TCD.
		struct TCDChannelUARTConfiguration
		{
			uint32 baudrate;					///<Baud rate of UART mode.
			uchar bitsperword;				///<Number of bits per word. Chosen from 7 or 8.
			uchar stopbits;					///<Number of stop bits. Chosen from 1 or 2.
			uchar parity;							///<Parity. Chosen from: None = 0, ODD = 1, EVEN = 2, PARITY_MARK = 3, PARITY_SPACE = 4.
														
			uint32 flowcontrol;				///<Flow control. Chosen from None = 0x0000, RTS_CTS = 0x0100, DTR_DSR = 0x0200, XON_XOFF = 0x0400.
		};

		//!Struct TCDChannelCommonConfigHint helps describing struct TCDChannelCommonConfig.
		struct TCDChannelCommonConfigHint
		{
			char USBBufferDescription[256];
			char readtimeoutsDescription[256];
			char writetimeoutsDescription[256];
			char retrycountDescription[256];
			char latencytimeDescription [256];
		};

		//!Struct TCDChannelI2CConfigHint helps describing struct TCDChannelI2CConfig.
		struct TCDChannelI2CConfigHint
		{
			char clockrateSupported[256];
			char clockrateDescription[256];
			char threephaseclockDescription[256];
			char slaveAddressDescription[256];
		};

		//!Struct TCDChannelUARTConfigHint helps describing struct TCDChannelUARTConfig.
		struct TCDChannelUARTConfigHint
		{
			char baudrateDescription[256];
			char bitsperwordSupported[64];
			char stopbitsSupported[64];
			char paritySupported[64];
			char flowcontrolSupported[64];
		};

		typedef TCDChannelInfo* pTCDChannelInfo;
		typedef TCDChannelCommonConfiguration* pTCDChannelCommonConfiguration;
		typedef TCDChannelI2CConfiguration* pTCDChannelI2CConfiguration;
		typedef TCDChannelUARTConfiguration* pTCDChannelUARTConfiguration;
		typedef TCDChannelCommonConfigHint* pTCDChannelCommonConfigHint;
		typedef TCDChannelI2CConfigHint* pTCDChannelI2CConfigHint;
		typedef TCDChannelUARTConfigHint* pTCDChannelUARTConfigHint;

		//!Error code used by TuningChannelDriver
	
		enum
		{
			TCD_FORMAT_I2C = 0x01,
			TCD_FORMAT_UART = 0x02,
			TCD_FORMAT_RESERVED = 0x04
		};

		enum
		{
			TCD_I2C_CLOCK_STANDARD_MODE			= 100000,	
			TCD_I2C_CLOCK_STANDARD_MODE_3P	= 133333,
			TCD_I2C_CLOCK_FAST_MODE					= 400000,				
			TCD_I2C_CLOCK_FAST_MODE_PLUS			= 1000000, 	
			TCD_I2C_CLOCK_HIGH_SPEED_MODE		= 3400000 	
		};

		//Baud rate
		enum {
		TCD_BAUD_300			= 300,
		TCD_BAUD_600			= 600,
		TCD_BAUD_1200		= 1200,
		TCD_BAUD_2400		= 2400,
		TCD_BAUD_4800		= 4800,
		TCD_BAUD_9600		= 9600,
		TCD_BAUD_14400		= 14400,
		TCD_BAUD_19200		= 19200,
		TCD_BAUD_38400		= 38400,
		TCD_BAUD_57600		= 57600,
		TCD_BAUD_115200	= 115200,
		TCD_BAUD_230400	=	230400,
		TCD_BAUD_460800	=	460800,
		TCD_BAUD_921600	=	921600
		};
	//
	// Word Lengths
	//
	enum 
	{	
		TCD_BITS_7 = 7	,		
		TCD_BITS_8 = 8			
	};

	//
	// Stop Bits
	//
	enum {
		TCD_STOP_BITS_1 = 0,
		TCD_STOP_BITS_2 = 2
	};

	//
	// Parity
	//
	enum {
	TCD_PARITY_NONE = 0,
	TCD_PARITY_ODD,
	TCD_PARITY_EVEN,
	TCD_PARITY_MARK,
	TCD_PARITY_SPACE
	};
	//
	// Flow Control
	//
	enum {
	TCD_FLOW_NONE = 0x0000,
	TCD_FLOW_RTS_CTS = 0x0100,
	TCD_FLOW_DTR_DSR = 0x0200,
	TCD_FLOW_XON_XOFF = 0x400
	};

	}

	namespace TCDCommunication
	{

	}
}
