#include "TCD_FTDICHIP_ChannelConfiguration.h"
#pragma comment(lib, "ftd2xx.lib")
#include <stdlib.h>
#include <vector>
#include <string>
#include <fstream>

#include <Windows.h>
#include "ftd2xx.h"
#include "libMPSSE_i2c.h"

using std::fstream;
using std::ofstream;
using std::ifstream;

static bool CopyConfiguration(pTCDChannelCommonConfiguration pDes, const pTCDChannelCommonConfiguration pSrc);
static bool CopyConfiguration( pTCDChannelI2CConfiguration pDes, const pTCDChannelI2CConfiguration pSrc);
static bool CopyConfiguration(pTCDChannelUARTConfiguration pDes, const pTCDChannelUARTConfiguration pSrc);
static bool CopyConfiguration(pTCDChannelCommonConfiguration pDes, TCDChannelCommonConfiguration Src);
static bool CopyConfiguration(pTCDChannelI2CConfiguration pDes, TCDChannelI2CConfiguration Src);
static bool CopyConfiguration(pTCDChannelUARTConfiguration pDes, TCDChannelUARTConfiguration Src);

static void GetErrorHintString(int erroType, std::string &resultHint);
static bool IsSingleBitOnByte(uchar oneByte);

static inline int CheckError(PVOID expression)
{
		if (NULL == expression)
		{
			return -1;
		}

		return 0;
}

namespace {	
		HMODULE h_libMPSSE = NULL;	//lib of MPSSE, used for I2C
		const uchar AVAILABLE_FORMAT = TCD_FORMAT_I2C | TCD_FORMAT_UART;
		
		//default format
		const uchar default_format = TCD_FORMAT_I2C;

		enum {
			DEFAULT_ADDRESS = 0x60 //default address of fortermedia chip
		};

		//default common configuration.
		const struct TCDChannelCommonConfiguration default_CommonConfiguration
			= { 4096,				//uint32 USBBuffer;		
				5000,				//uint32 readtimeouts;	
				1000,				//uint32 writetimeouts;
				10,					//uint32 retrycount;		
				16						//uint32 latencytime;	
		};
		
		enum {
			CONFIGTYPE_FORMAT,
			CONFIGTYPE_COMMON,
			CONFIGTYPE_UART,
			CONFIGTYEP_I2C,
			CONFIGTYPE_SERIALNUMBER
		};

		//If you want to add item,  insert it into specified type. not be on top or bottom, and the same with string
		enum {
			//Common config
			USBuffer_ERROR,
			readwritetimeouts_ERROR,
			writetimeouts_ERROR,
			retrycount_ERROR,
			latencytime_ERROR,
			
			//I2C config
			clockrate_ERROR,
			slaveaddress_ERROR,
			threephaseclock_ERROR,

			//UART Config
			baudrate_ERROR,
			bitsperword_ERROR,
			stopbits_ERROR,
			parity_ERROR,
			flowcontrol_ERROR
		};

		struct  configStruct
		{
			//Use for error code.
			int error_code;
			std::string strError;

			//Use for config file.
			std::string configKeyWords;	//<Key words in configuration file.
			int valueType;						//<value type to identify if its number or string.
			enum {
				TYPE_NUMBER,
				TYPE_STRING,
				TYPE_BOOL
			};
		};
		
		const char const_serialNumberKeyWords[] = "SERIAL_NUMBER";
		const char const_currentFormatKeyWords[] = "CURRENT_FORMAT";

		const struct configStruct configStructMap[] = {
			{USBuffer_ERROR, "USB buffer configuration error.", "USB_BUFFER"},
			{readwritetimeouts_ERROR, "Read time outs error.", "READ_TIME_OUTS"},
			{writetimeouts_ERROR, "Write time outs error.", "WRITE_TIME_OUTS"},
			{retrycount_ERROR, "Retry counts error.", "RETRY_COUNT"},
			{latencytime_ERROR, "Latency timer error.", "LATENCY_TIME"},
			
			{clockrate_ERROR, "Clock rate error", "CLOCK_RATE"},
			{slaveaddress_ERROR, "slave address error", "SLAVE_ADDRESS"},
			{threephaseclock_ERROR, "Three phase clock error.", "THREE_PHASE_CLOCK"},
			

			{baudrate_ERROR, "Baud rate error.", "BAUD_RATE"},
			{bitsperword_ERROR, "bits per word error.", "BITS_PERWORD"},
			{stopbits_ERROR, "Stop bits error.", "STOPBITS"},
			{parity_ERROR, "Parity error.", "PARITY"},
			{flowcontrol_ERROR, "Flow control error.", "FLOW_CONTROL"}
		};

		//default I2C configuration.
		const struct TCDChannelI2CConfiguration default_I2CConfiguration
			= { TCD_I2C_CLOCK_FAST_MODE,		//clockrate.STANDARD
					false,			//three phase clock.
					DEFAULT_ADDRESS
		} ;

		//default UART configuration.
		const struct TCDChannelUARTConfiguration default_UART_Configuration
			= { TCD_BAUD_19200,		//baud rate.
				TCD_BITS_8,					//bits per word.
				TCD_STOP_BITS_1,					//stop bits.
				TCD_PARITY_NONE,/*"None",*/		//parity
				TCD_FLOW_NONE/*"None"*/			//flow control
		};

		//common configuration hint.
		const struct TCDChannelCommonConfigHint const_Common_Config_Hint
			= {
				"Default USB buffer is 4096(byes), it must be set to a multiple of 64 bytes between 64bytes and 64k bytes",	//USB Buffer
				"Read time outs in milliseconds, default to be 5000",
				"Write time outs in milliseconds, default to be 1000",
				"Number of times that the driver try to reset a pipe on which an error has occurred, default to be 50."
				"Timer in millisecond to refresh receive buffer."
		};

		//I2C configuration hint.
		const struct TCDChannelI2CConfigHint const_I2C_Config_Hint
			= {
				"Standard Mode, Standard Mode 3P, Fast Mode, Fast Mode Plus, High Speed Mode",//clockrate supported
				"Standard Mode = 100kb/sec, Standard Mode 3p = 133.3kb/sec, Fast Mode = 400kb/sec, Fast Mode Plus = 1Mb/sec, High Speed Mode = 3.4Mb/sec"
				"TODO://I don't know what 3-phase clock condition is.",
				"TODO://Slave address description. "
		};

		//UART configuration hint.
		const struct TCDChannelUARTConfigHint const_UART_Config_Hint
			= {
				"Baud rate, default to be 192000",
				"7, 8",
				"1, 2",
				"None, ODD, EVEN, PARITY_MARK, PARITY_SPACE",//char paritySupported[64];
				"None, RTS_CTS, DTR_DSR, XON_XOFF"//char flowcontrolSupported[64];
		};
		
		typedef FT_STATUS (*pfunc_I2C_InitChannel)(FT_HANDLE handle, ChannelConfig \
			*config);
		pfunc_I2C_InitChannel p_I2C_InitChannel;

		typedef FT_STATUS (*pfunc_I2C_OpenChannel)(FT_HANDLE handle, ChannelConfig \
			*config);
		pfunc_I2C_OpenChannel p_I2C_OpenChannel;

		typedef FT_STATUS (*pfunc_I2C_CloseChannel)(FT_HANDLE handle, ChannelConfig \
			*config);

			namespace{

		static int StringToNumber(std::string strContent)
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

		static int NumberToString(int Number, std::string &strNumber)
		{
			strNumber.clear();
			if (Number < 0)
			{
				return -1;
			}

			std::string strResult;

			if (0 == Number)
			{
				strNumber.push_back('0');
				return 0;
			}

			while (Number != 0)
			{
				char appendNumber = Number%10 + '0';
				Number /= 10;
				 
				strResult.push_back(appendNumber);
			}
			//Reverse string
			
			std::string::reverse_iterator  riterstrNumber= strResult.rbegin();
			for (; riterstrNumber != strResult.rend( ); riterstrNumber++ )
				strNumber.push_back(*riterstrNumber);
			return 0;
		}

		static bool StringToBool(std::string strContent)
		{
			const std::string strTrue_m = "TRUE";
			const std::string strTruel_n = "true";
			const std::string strFalse_m = "FALSE";
			const std::string strFalse_n = "false";
			if(0 == strContent.compare(strTrue_m) || 0 == strContent.compare(strTruel_n))
			{
				return true;
			}

			if (0 == strContent.compare(strFalse_m) || 0 == strContent.compare(strFalse_n))
			{
				return false;
			}

			return false;
		}

		static int StringToNumber(std::string strContent);
		static bool StringToBool(std::string strContent);
		static int NumberToString(int Number, std::string &strNumber);
		static int getExactContent(std::string strEachLine, std::string strKeyWord, std::string &strContent);
		class TCDConfigLog {
		public:
			static int SetPath(std::string strPath);
			static bool Open();
			static void Close();
			//Load serial number only used when each tcd is wit single config file 
			static int LoadSerialNumber(std::string &strSerialNumber);
			static int SaveSerialNumber(std::string strSerialNumber);

			static int LoadCurrentFormat(std::string strSerialNumber, puchar pFormat);
			static int LoadCommonConfiguration(pTCDChannelCommonConfiguration pcommonConfig, std::string strSerialNumber);
			static int LoadI2CConfiguration(pTCDChannelI2CConfiguration pI2CConfig, std::string strSerialNumber);
			static int LoadUARTConfiguration(pTCDChannelUARTConfiguration pUARTConfig, std::string strSerialNumber);

			static int SaveCurrentFormat(std::string strSerialNumber, uchar Format);
			static int SaveCommonConfiguration(pTCDChannelCommonConfiguration pcommonConfig, std::string strSerialNumber);
			static int SaveI2CConfiguration(pTCDChannelI2CConfiguration pI2CConfig, std::string strSerialNumber);
			static int SaveUARTConfiguration(pTCDChannelUARTConfiguration pUARTConfig, std::string strSerialNumber);
		
			static int InsertItem(std::string strSerialNumber);
			static int DeleteItem(std::string strSerialNumber);
			static const std::string m_const_logFilePath;//= "Option.ini";
		private:	
			static bool FindContent(std::string strSerialNumber, std::string strKeyWord, std::string &strContent);
			static bool ChangContent(std::string strSerialNumber, std::string strKeyWord, std::string strContent);
			static fstream m_logfile;
			
			static const std::string m_strSpace; //(" = ");
			static const std::string m_strBlank; //("#----------------------------------------");
			static const std::string m_strCommonP;// #Common Parameters
			static const std::string m_strI2CP;// #I2C Parameters
			static const std::string m_strUARTP;// #UART Parameters
			static const std::string m_strFileHead;
			static const std::string m_strTempFilePath;
			static std::string TCDConfigLog::m_logFilePath;
			//Single file means all configurations are in the same file.
			static bool m_b_LOG_SINGLEFILE;
			enum {
				MAX_CHARACTERS_EACH_LINE = 256
			};


		};
		fstream TCDConfigLog::m_logfile;
		bool TCDConfigLog::m_b_LOG_SINGLEFILE = false;
		std::string TCDConfigLog::m_logFilePath;
		const std::string TCDConfigLog::m_strFileHead = "#This file stores configurations for debug tools.\
																					\n#Don't change this file unless you are very certain about it.\
																					\n#It's Ok to delete this file and all channels' configurations return to default.\
																					\n#---------------------------------------------------------------------------";
		const std::string TCDConfigLog::m_const_logFilePath = "Option.ini";
		const std::string TCDConfigLog::m_strSpace = " = ";
		const std::string TCDConfigLog::m_strBlank = "#----------------------------------------";
		const std::string TCDConfigLog::m_strCommonP = "#Common Parameters";
		const std::string TCDConfigLog::m_strI2CP = "#I2C Parameters";
		const std::string TCDConfigLog::m_strUARTP = "#UART Parameters\n#STOPBITS: 0~Stop bit 1, 2~Stop bit 2;\
													 \n#PARITY: 0~None, 1~ODD, 2~EVEN, 3~MARK, 4~SPACE\
													 \n#FLOW_CONTROL: 0~None, 256(0x0100)~RTC_CTS, 512(0x200)~DTR_DSR, 1024~XON/XOFF";

		const std::string TCDConfigLog::m_strTempFilePath = "tmp.tmp";

		int TCDConfigLog::SetPath(std::string strPath)
		{
			m_logFilePath.assign(strPath);
			return 0;
		}

		//Load serial number only used when each tcd is wit single config file 
		int TCDConfigLog::LoadSerialNumber(std::string &strSerialNumber)
		{
			if (true == m_b_LOG_SINGLEFILE)
			{
				return 0;
			}
			strSerialNumber.clear();
			if (false == FindContent(strSerialNumber, const_serialNumberKeyWords, strSerialNumber))
			{
				return -1;
			}
			return 0;
		}

		//TCDConfigLog::fstream logFile();
		int TCDConfigLog::LoadCommonConfiguration(pTCDChannelCommonConfiguration pcommonConfig, std::string strSerialNumber)
		{
			for (int i = USBuffer_ERROR; i != latencytime_ERROR + 1; ++i)
			{
				std::string strConfigContent;
				if(false == FindContent(strSerialNumber, configStructMap[i].configKeyWords, strConfigContent))
				{
					return -2;
				}

				int number = StringToNumber(strConfigContent);
				bool flag = false;

				switch(configStructMap[i].error_code)
				{
				case USBuffer_ERROR:
					pcommonConfig->USBBuffer = number;
					break;
				case readwritetimeouts_ERROR:
					pcommonConfig->readtimeouts = number;
					break;
				case writetimeouts_ERROR:
					pcommonConfig->writetimeouts = number;
					break;
				case	retrycount_ERROR:
					pcommonConfig->retrycount = number;
					break;
				case latencytime_ERROR:
					pcommonConfig->latencytime = number;
					break;
				default:
					break;
				}
			}

			return 0;
		}

		int TCDConfigLog::LoadI2CConfiguration(pTCDChannelI2CConfiguration pI2CConfig, std::string strSerialNumber)
		{
			for (int i= clockrate_ERROR; i != threephaseclock_ERROR + 1; ++i)
			{
				std::string strConfigContent;
				if(false == FindContent(strSerialNumber, configStructMap[i].configKeyWords, strConfigContent))
				{
					return -2;
				}

				int number = StringToNumber(strConfigContent);
				bool flag = false;
				switch(configStructMap[i].error_code)
				{
				case clockrate_ERROR:
					pI2CConfig->clockrate = number;
					break;

				case threephaseclock_ERROR:
					if (0 != number)
					{
						pI2CConfig->threephaseclock = true;
					}
					else
					{
						pI2CConfig->threephaseclock = false;
					}
					break;
				default:
					break;
				}
			}

			return 0;
		}

		int TCDConfigLog::LoadUARTConfiguration(pTCDChannelUARTConfiguration pUARTConfig, std::string strSerialNumber)
		{
			for (int i=baudrate_ERROR; i != flowcontrol_ERROR + 1; ++i)
			{
				std::string strConfigContent;
				if(false == FindContent(strSerialNumber, configStructMap[i].configKeyWords, strConfigContent))
				{
					return -2;
				}

				int number = StringToNumber(strConfigContent);
				bool flag = false;
				char* stringAddr = NULL;

				switch(configStructMap[i].error_code)
				{
				case baudrate_ERROR:
					pUARTConfig->baudrate = number;
					break;
				case bitsperword_ERROR:
					pUARTConfig->bitsperword = number;
					break;
				case flowcontrol_ERROR:
					pUARTConfig->flowcontrol = number;
					break;
				case	parity_ERROR:
					pUARTConfig->parity = number;
					break;
				case stopbits_ERROR:
					pUARTConfig->stopbits = number;
					break;
				default:
					break;
				}
			}

			return 0;
		}

		int TCDConfigLog::SaveSerialNumber(std::string strSerialNumber)
		{
			if (true == m_b_LOG_SINGLEFILE)
			{
				return 0;
			}

			strSerialNumber.clear();
			InsertItem(strSerialNumber);
			if (false == ChangContent(strSerialNumber, const_serialNumberKeyWords, strSerialNumber))
			{
				return -1;
			}
			return 0;
		}

		int TCDConfigLog::SaveCommonConfiguration(pTCDChannelCommonConfiguration pcommonConfig, std::string strSerialNumber)
		{
			/*
			USBuffer_ERROR,
			readwritetimeouts_ERROR,
			writetimeouts_ERROR,
			retrycount_ERROR,
			latencytime_ERROR,
			*/
			for (int i = USBuffer_ERROR; i != latencytime_ERROR + 1; ++i)
			{
				std::string strContent;
				int num = 0;
				bool flag = false;
				char* stringAddr = NULL;
				  
				switch (i)
				{
				case USBuffer_ERROR:
					num = pcommonConfig->USBBuffer;
					break;
				case readwritetimeouts_ERROR:
					num = pcommonConfig->readtimeouts;
					break;
				case	writetimeouts_ERROR:
					num = pcommonConfig->writetimeouts;
					break;
				case	retrycount_ERROR:
					num = pcommonConfig->retrycount;
					break;
				case	latencytime_ERROR:
					num = pcommonConfig->latencytime;
					break;
				default:
					break;
					return -1;
				}

				if (configStruct::TYPE_NUMBER == configStructMap[i].valueType)
				{
					NumberToString(num, strContent);

				} else if (configStruct::TYPE_BOOL == configStructMap[i].valueType)
				{
					NumberToString(flag, strContent);
				} else if (configStruct::TYPE_STRING == configStructMap[i].valueType)
				{
					strContent.assign(stringAddr);
				}

				ChangContent(strSerialNumber, configStructMap[i].configKeyWords, strContent);
			}
			return 0;
		}

		int TCDConfigLog::SaveI2CConfiguration(pTCDChannelI2CConfiguration pI2CConfig, std::string strSerialNumber)
		{
			for (int i=clockrate_ERROR; i != threephaseclock_ERROR+1; ++i)
			{
				std::string strContent;
				int num = 0;
				bool flag = false;
				char* stringAddr = NULL;
				switch (i)
				{
				case clockrate_ERROR:
					num = pI2CConfig->clockrate;
					break;
				case threephaseclock_ERROR:
					flag = pI2CConfig->threephaseclock;
					break;
				case slaveaddress_ERROR:
					num = pI2CConfig->slaveaddress;
					break;
				default:
					break;
				}

				if (configStruct::TYPE_NUMBER == configStructMap[i].valueType)
				{
					NumberToString(num, strContent);

				} else if (configStruct::TYPE_BOOL == configStructMap[i].valueType)
				{
					NumberToString(flag, strContent);
				} else if (configStruct::TYPE_STRING == configStructMap[i].valueType)
				{
					strContent.assign(stringAddr);
				}

				ChangContent(strSerialNumber, configStructMap[i].configKeyWords, strContent);
			}

			return 0;
		}

		int TCDConfigLog::SaveUARTConfiguration(pTCDChannelUARTConfiguration pUARTConfig, std::string strSerialNumber)
		{
			for (int i=baudrate_ERROR; i != flowcontrol_ERROR+1; ++i)
			{
				std::string strContent;
				int num = 0;
				bool flag = false;
				char* stringAddr = NULL;

				switch (i)
				{
				case baudrate_ERROR:
					num = pUARTConfig->baudrate;
					break;
				case stopbits_ERROR:
					num = pUARTConfig->stopbits;
					break;
				case parity_ERROR:
					num = pUARTConfig->parity;
					break;
				case flowcontrol_ERROR:
					num = pUARTConfig->flowcontrol;
					break;
				case bitsperword_ERROR:
					num = pUARTConfig->bitsperword;
					break;
				default:
					break;
				}

				if (configStruct::TYPE_NUMBER == configStructMap[i].valueType)
				{
					NumberToString(num, strContent);

				} else if (configStruct::TYPE_BOOL == configStructMap[i].valueType)
				{
					NumberToString(flag, strContent);
				} else if (configStruct::TYPE_STRING == configStructMap[i].valueType)
				{
					strContent.assign(stringAddr);
				}

				ChangContent(strSerialNumber, configStructMap[i].configKeyWords, strContent);
			}

			return 0;
		}

		int TCDConfigLog::LoadCurrentFormat(std::string strSerialNumber, puchar pFormat)
		{


			std::string strConfigContent;
			if(false == FindContent(strSerialNumber, const_currentFormatKeyWords, strConfigContent))
			{
				return -2;
			}
			*pFormat = StringToNumber(strConfigContent);
			//memcpy(pFormat, strConfigContent.c_str(), sizeof(uchar));
			return 0;
		}

		int TCDConfigLog::SaveCurrentFormat(std::string strSerialNumber, uchar Format)
		{
			//
			//std::string strContent;
			//strContent.assign(const_currentFormatKeyWords);
			std::string strContent;
			strContent.push_back('0' + Format);

			ChangContent(strSerialNumber, const_currentFormatKeyWords, strContent);
			return 0;
		}

		//If SN. is null find the first key words.

		bool TCDConfigLog::FindContent(std::string strSerialNumber, std::string strKeyWord, std::string &strContent)
		{
			bool bSNFound = false;
			char eachLine[MAX_CHARACTERS_EACH_LINE];
			m_logfile.seekg(0, std::ios::beg);
			while (!m_logfile.eof())
			{
				m_logfile.getline(eachLine, sizeof(eachLine));
				std::string stringeachLine(eachLine);
				if ('#' == *eachLine)
				{
					continue;
				}

				if (false == bSNFound)
				{
					//S.N. empty, find the first configurations for
					/*if (strSerialNumber.empty())
					{
						bSNFound = true;
						continue;
					}*/
					if (false == m_b_LOG_SINGLEFILE)
					{
						bSNFound = true;
						if(0 == getExactContent(stringeachLine, strKeyWord, strContent))
							return true;
						continue;
					}

					//Find serial number
					if (-1 != stringeachLine.find(strSerialNumber, 0))
					{
						bSNFound = true;
						continue;
					}			
				}
				
				if (true == bSNFound)
				{
					if(0 == getExactContent(stringeachLine, strKeyWord, strContent))
						return true;
				}
			}
			return false;
		}

		int TCDConfigLog::InsertItem(std::string strSerialNumber)
		{
			//if (false == m_b_LOG_SINGLEFILE)
			//{
			//	return 0;
			//}

			Close();

			if (false == m_b_LOG_SINGLEFILE)
			{
				printf("%s", m_logFilePath.c_str());
				m_logfile.open(m_logFilePath.c_str(), std::ios::in | std::ios::out | std::ios::trunc);
			}
			else 
			{
				m_logfile.open(m_logFilePath.c_str(), std::ios::app | std::ios::out);
			}
			
			if(!m_logfile.is_open())
			{
				return -1;
			}

			std::string strLine;
			//Insert blank line

			m_logfile<<m_strBlank<<std::endl;
			//Insert SerialNumber
			strLine.assign(const_serialNumberKeyWords);
			strLine.insert(strLine.end(), m_strSpace.begin(), m_strSpace.end());
			strLine.insert(strLine.end(), strSerialNumber.begin(), strSerialNumber.end());
		
			m_logfile<<strLine<<std::endl;

			//Insert  current format line
			strLine.clear();
			strLine.assign(const_currentFormatKeyWords);
			strLine.insert(strLine.end(), m_strSpace.begin(), m_strSpace.end());
			m_logfile<<strLine<<std::endl;

			//Insert -----------
			m_logfile<<m_strBlank<<std::endl;

			//insert key words of common configuration
			for (int i= USBuffer_ERROR; i != flowcontrol_ERROR + 1; ++i)
			{
				strLine.clear();
				if (USBuffer_ERROR == i)
				{
					//Insert -----------
					m_logfile<<m_strBlank<<std::endl;
					//inset common parameters
					strLine.assign(m_strCommonP);
					m_logfile<<strLine<<std::endl;
				}

				if (clockrate_ERROR == i)
				{
					//Insert -----------
					m_logfile<<m_strBlank<<std::endl;
					//inset common paramters
					strLine.assign(m_strI2CP);
					m_logfile<<strLine<<std::endl;
				}

				if (baudrate_ERROR == i)
				{
					//Insert -----------
					m_logfile<<m_strBlank<<std::endl;
					//inset common paramters
					strLine.assign(m_strUARTP);
					m_logfile<<strLine<<std::endl;
				}

				strLine.assign(configStructMap[i].configKeyWords);
				strLine.insert(strLine.end(), m_strSpace.begin(), m_strSpace.end());
				m_logfile<<strLine<<std::endl;
			}
			TCDConfigLog::SaveCurrentFormat(strSerialNumber, default_format);
			TCDChannelCommonConfiguration tmp_defaultCommonConfig;
			TCDChannelUARTConfiguration tmp_defaultUARTConfig;
			TCDChannelI2CConfiguration tmp_defaultI2CConfig;

			CopyConfiguration(&tmp_defaultCommonConfig, default_CommonConfiguration);
			CopyConfiguration(&tmp_defaultI2CConfig, default_I2CConfiguration);
			CopyConfiguration(&tmp_defaultUARTConfig, default_UART_Configuration);
			

			TCDConfigLog::SaveCommonConfiguration(&tmp_defaultCommonConfig, strSerialNumber);
			TCDConfigLog::SaveI2CConfiguration(&tmp_defaultI2CConfig, strSerialNumber);
			TCDConfigLog::SaveUARTConfiguration(&tmp_defaultUARTConfig, strSerialNumber);
			return 0;
		}

		int TCDConfigLog::DeleteItem(std::string strSerialNumber)
		{
			Close();
			if (!Open())
			{
				return false;
			}
			m_logfile.seekg(0, std::ios::beg);
			std::string strTempContent;
			//Check if exists.
			bool bSNFound = false;
			char eachLine[MAX_CHARACTERS_EACH_LINE];
			ofstream oftmp;
			oftmp.open(m_strTempFilePath.c_str(),std::ios::out| std::ios::trunc);

			if(!oftmp.is_open())
			{
				return false;
			}

			bool flagDeleteFinish = false;
			while (m_logfile.getline(eachLine, sizeof(eachLine)))
			{
				std::string stringeachLine(eachLine);

				if (true == flagDeleteFinish)
				{
					oftmp<<stringeachLine<<std::endl;
					continue;
				}

				if ('#' == *eachLine)
				{
					oftmp<<stringeachLine<<std::endl; //write.
					continue;
				}

				if (false == bSNFound)
				{
					
					//Find serial number
					if (-1 != stringeachLine.find(strSerialNumber, 0))
					{
						bSNFound = true;
					}

					oftmp<<stringeachLine<<std::endl; 
				}

				if (true == bSNFound)
				{
					if (-1 != stringeachLine.find(const_serialNumberKeyWords, 0))
					{
						//Find another SN to finish delete function
						flagDeleteFinish = true;
						oftmp<<stringeachLine<<std::endl;
					}
				}
		
			}

			oftmp.close();
			oftmp.clear();
			m_logfile.close();
			m_logfile.clear();

			oftmp.open(m_logFilePath.c_str(),std::ios::out | std::ios::trunc);
			if(!oftmp.is_open())
			{
				return false;
			}

			ifstream iftmp;
			iftmp.open(m_strTempFilePath.c_str(),std::ios::in);
			if(!iftmp.is_open())
			{
				return false;
			}

			while(iftmp.getline(eachLine, sizeof(eachLine)))
			{
				oftmp <<eachLine << std::endl;
			}

			iftmp.close();
			iftmp.clear();
			oftmp.close();
			oftmp.clear();
			remove(m_strTempFilePath.c_str());
			m_logfile.open(m_logFilePath.c_str(), std::ios::in | std::ios::out);

			return 0;
		}

		bool TCDConfigLog::ChangContent(std::string strSerialNumber, std::string strKeyWord, std::string strContent)
		{
			Close();
			if (!Open())
			{
				return false;
			}
			m_logfile.seekg(0, std::ios::beg);
 			std::string strTempContent;
			//Check if exists.
			bool bSNFound = false;
			char eachLine[MAX_CHARACTERS_EACH_LINE];
 			ofstream oftmp;
			oftmp.open(m_strTempFilePath.c_str(),std::ios::out| std::ios::trunc);

			if(!oftmp.is_open())
			{
				return false;
			}

			while (m_logfile.getline(eachLine, sizeof(eachLine)))
			{
				std::string stringeachLine(eachLine);
				if ('#' == *eachLine)
				{
					oftmp<<stringeachLine<<std::endl; //write.
					continue;
				}

				if (false == bSNFound)
				{
					oftmp<<stringeachLine<<std::endl; 
					
					if (false == m_b_LOG_SINGLEFILE)
					{
						bSNFound = true;
						continue;
					}
					//Find serial number
					if (-1 != stringeachLine.find(strSerialNumber, 0))
					{
						bSNFound = true;
						continue;
					}			
				}

				if (true == bSNFound)
				{
					std::string stringTemp;
					bool alreadyFound = false;
					if(!alreadyFound && 0 == getExactContent(stringeachLine, strKeyWord, stringTemp))
						{
							//Write my own content line
							oftmp<<strKeyWord<<m_strSpace<<strContent<<std::endl;
							alreadyFound = true;
							continue;
						}
					oftmp<<stringeachLine<<std::endl;
				}
			}
			
			oftmp.close();
			oftmp.clear();
 			m_logfile.close();
			m_logfile.clear();

			oftmp.open(m_logFilePath.c_str(),std::ios::out | std::ios::trunc);
			if(!oftmp.is_open())
			{
				return false;
			}

			ifstream iftmp;
			iftmp.open(m_strTempFilePath.c_str(),std::ios::in);
			if(!iftmp.is_open())
			{
				return false;
			}

			while(iftmp.getline(eachLine, sizeof(eachLine)))
			{
				oftmp <<eachLine << std::endl;
			}

			iftmp.close();
			iftmp.clear();
			oftmp.close();
			oftmp.clear();
			remove(m_strTempFilePath.c_str());
			m_logfile.open(m_logFilePath.c_str(), std::ios::in | std::ios::out);
			return true;
		}

		//Open file, if file not exists, create one
		bool TCDConfigLog::Open()
		{
			//f.open("d:\\12.txt",ios::out);
			if (m_logFilePath.empty())
			{
				m_logFilePath.assign(m_const_logFilePath);
			}

			m_logfile.open(m_logFilePath.c_str(), std::ios::in | std::ios::out | std::ios::_Nocreate);
			if (m_logfile.is_open())
			{
				return true;
			}
			
			//Create file if not exists.
			m_logfile.open(m_logFilePath.c_str(), std::ios::in | std::ios::out|std::ios::trunc);
			if (m_logfile.is_open())
			{
				m_logfile<<m_strFileHead<<std::endl;
				return true;
			}
			
			return false;
		}

		void TCDConfigLog::Close()
		{
			m_logfile.close();
		}

		//Used for TCDConfigLog
		static int getExactContent(std::string strEachLine, std::string strKeyWord, std::string &strContent)
		{
			strContent.clear();
			std::string::size_type pos = strEachLine.find(strKeyWord, 0);
			if (-1 == pos)
			{
				return -1;
			}
			//find now.
			strEachLine.erase(0, pos + strKeyWord.size());
			pos = strEachLine.find_first_of('=', 0);
			if (-1 == pos)
			{
				return -1;
			}

			strEachLine.erase(0, pos + 1);
			//find '=' now.
			std::string::iterator iterEachLine = strEachLine.begin();
			for (; iterEachLine != strEachLine.end(); ++iterEachLine)
			{
				if (' '== *iterEachLine)
				{
					continue;
				}
				strContent.push_back(*iterEachLine);
			}
			return 0;
		}
	};
}

TCD_FTDICHIP_ChannelConfiguration::TCD_FTDICHIP_ChannelConfiguration(TCDHandle tcdHandle)
{
	m_address = DEFAULT_ADDRESS;
	m_tcdHandle = tcdHandle;
}


TCD_FTDICHIP_ChannelConfiguration::~TCD_FTDICHIP_ChannelConfiguration(void)
{
	FreeLibrary(h_libMPSSE);
}

uint32 TCD_FTDICHIP_ChannelConfiguration::Initial()
{
	h_libMPSSE = LoadLibrary("libMPSSE.dll");
	if (NULL == h_libMPSSE)
	{
		printf("Failed loading libMPSSE.dll. Please check if the file exists in\
			   the working directory\n");
		return TCD_OTHER_ERROR;
	}

	p_I2C_InitChannel = (pfunc_I2C_InitChannel)GetProcAddress(h_libMPSSE\
		, "I2C_InitChannel");
	if (0 != CheckError(p_I2C_InitChannel))
	{
		return TCD_FAIL_INITIAL;
	}

	//Load configuration from file into class members
	if (0 != LoadConfigurationFromFile(m_serialNumber))
	{
		LoadDefaultConfiguration();
	}

	//We need to check if configuration are available
	std::string strTemp;
	uint32 result = 0;
	
	result |= CheckChannelCommConfiguration(&m_channelCommConfig, strTemp);
	result |= CheckChannelI2CConfiguration(&m_channelI2CConfig, strTemp);
	result |= CheckChannelUARTConfiguration(&m_channelUARTConfig, strTemp);

	if (0 != result)
	{
		LoadDefaultConfiguration();
	}
	//
	//Other Initialization
	//Initial I2C State if current format is I2C
	if (TCD_FORMAT_I2C == m_format)
	{
		FT_STATUS ftStatus = FT_OK;
		ChannelConfig channel_config;
		channel_config.ClockRate = (I2C_ClockRate_t)m_channelI2CConfig.clockrate;
		channel_config.LatencyTimer = m_channelCommConfig.latencytime;
		channel_config.Options = I2C_DISABLE_3PHASE_CLOCKING;

		if (false == m_channelI2CConfig.threephaseclock)
		{
			channel_config.Options = I2C_DISABLE_3PHASE_CLOCKING;
		} 

		pfunc_I2C_CloseChannel p_I2C_CloseChannel;
		
		ftStatus = p_I2C_InitChannel(m_tcdHandle, &channel_config);
		if (FT_OK != ftStatus)
		{
			return TCD_FAIL_INITIAL;
		}
	}

	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::Reset()
{
	m_serialNumber.clear();
	LoadDefaultConfiguration();
	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::GetChannelAvailableFormat(puchar pformatByte)
{	
	if (CheckError(pformatByte))
	{
		return TCD_PARAM_ERROR;
	}

	memcpy(pformatByte, &AVAILABLE_FORMAT, sizeof(uchar));

	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::GetChannelFormat(puchar pformatByte)
{
	if (CheckError(pformatByte))
	{
		return TCD_PARAM_ERROR;
	}

	memcpy(pformatByte, &m_format, sizeof(uchar));

	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::GetChannelCommonConfiguration(pTCDChannelCommonConfiguration pcommonConfig)
{
	if (CheckError(pcommonConfig))
	{
		return TCD_PARAM_ERROR;
	}

	if (!CopyConfiguration(pcommonConfig, &m_channelCommConfig))
	{
		return TCD_OTHER_ERROR;
	}

	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::GetChannelI2CConfiguration(pTCDChannelI2CConfiguration pI2CConfig)
{
	if (CheckError(pI2CConfig))
	{
		return TCD_PARAM_ERROR;
	}

	if (!CopyConfiguration(pI2CConfig, &m_channelI2CConfig))
	{
		return TCD_OTHER_ERROR;
	}

	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::GetChannelUARTConfiguration(pTCDChannelUARTConfiguration pUARTConfig)
{
	if (CheckError(pUARTConfig))
	{
		return TCD_PARAM_ERROR;
	}

	if (!CopyConfiguration(pUARTConfig, &m_channelUARTConfig))
	{
		return TCD_OTHER_ERROR;
	}

	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::GetChannelCommonConfigurationHint(pTCDChannelCommonConfigHint pcommonConfigHint)
{
	if (CheckError(pcommonConfigHint))
	{
		return TCD_PARAM_ERROR;
	}

	memcpy_s(pcommonConfigHint->latencytimeDescription, sizeof(pcommonConfigHint->latencytimeDescription), const_Common_Config_Hint.latencytimeDescription, sizeof(pcommonConfigHint->latencytimeDescription));
	memcpy_s(pcommonConfigHint->readtimeoutsDescription, sizeof(pcommonConfigHint->readtimeoutsDescription), const_Common_Config_Hint.readtimeoutsDescription, sizeof(pcommonConfigHint->readtimeoutsDescription));
	memcpy_s(pcommonConfigHint->writetimeoutsDescription, sizeof(pcommonConfigHint->writetimeoutsDescription), const_Common_Config_Hint.writetimeoutsDescription, sizeof(pcommonConfigHint->writetimeoutsDescription));
	memcpy_s(pcommonConfigHint->retrycountDescription, sizeof(pcommonConfigHint->retrycountDescription), const_Common_Config_Hint.retrycountDescription, sizeof(pcommonConfigHint->retrycountDescription));
	memcpy_s(pcommonConfigHint->USBBufferDescription, sizeof(pcommonConfigHint->USBBufferDescription), const_Common_Config_Hint.USBBufferDescription, sizeof(pcommonConfigHint->USBBufferDescription));
	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::GetChannelI2CConfigurationHint(pTCDChannelI2CConfigHint pI2CConfigHint)
{
	if (CheckError(pI2CConfigHint))
	{
		return TCD_PARAM_ERROR;
	}

	memcpy_s(pI2CConfigHint->clockrateDescription, sizeof(pI2CConfigHint->clockrateDescription), const_I2C_Config_Hint.clockrateDescription,  sizeof(pI2CConfigHint->clockrateDescription));
	memcpy_s(pI2CConfigHint->clockrateSupported, sizeof(pI2CConfigHint->clockrateSupported), const_I2C_Config_Hint.clockrateSupported,  sizeof(pI2CConfigHint->clockrateSupported));
	memcpy_s(pI2CConfigHint->threephaseclockDescription, sizeof(pI2CConfigHint->threephaseclockDescription), const_I2C_Config_Hint.threephaseclockDescription,  sizeof(pI2CConfigHint->threephaseclockDescription));
	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::GetChannelUARTConfigurationHint(pTCDChannelUARTConfigHint pUARTConfigHint)
{
	if (CheckError(pUARTConfigHint))
	{
		return TCD_PARAM_ERROR;
	}

	memcpy_s(pUARTConfigHint->baudrateDescription, sizeof(pUARTConfigHint->baudrateDescription), const_UART_Config_Hint.baudrateDescription, sizeof(pUARTConfigHint->baudrateDescription));
	memcpy_s(pUARTConfigHint->bitsperwordSupported, sizeof(pUARTConfigHint->bitsperwordSupported), const_UART_Config_Hint.bitsperwordSupported, sizeof(pUARTConfigHint->bitsperwordSupported));
	memcpy_s(pUARTConfigHint->flowcontrolSupported, sizeof(pUARTConfigHint->flowcontrolSupported), const_UART_Config_Hint.flowcontrolSupported, sizeof(pUARTConfigHint->flowcontrolSupported));
	memcpy_s(pUARTConfigHint->paritySupported, sizeof(pUARTConfigHint->paritySupported), const_UART_Config_Hint.paritySupported, sizeof(pUARTConfigHint->paritySupported));
	memcpy_s(pUARTConfigHint->stopbitsSupported, sizeof(pUARTConfigHint->stopbitsSupported), const_UART_Config_Hint.stopbitsSupported, sizeof(pUARTConfigHint->stopbitsSupported));
	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::CheckChannelCommConfiguration(const pTCDChannelCommonConfiguration pcommonConfig, std::string &resultHint)
{
	if (CheckError(pcommonConfig))
	{
		return TCD_PARAM_ERROR;
	}

	//Copy configuration
	TCDChannelCommonConfiguration OldCommonConfiguration;
	CopyConfiguration(&OldCommonConfiguration, &m_channelCommConfig);

	int retVal = 0;
	//Set configuration to test if it's valid.
	if (0 != (retVal = SetChannelCommonConfiguration(pcommonConfig)))
	{
		//Configuration is not valid.
		GetErrorHintString(retVal, resultHint);
	}

	//Former configuration is not well valid? 
	if (0 != SetChannelCommonConfiguration(&OldCommonConfiguration))
	{
		//Former configuration is not valid. Serial number may be the same as the other one.
		return TCD_OTHER_ERROR;
	}
	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::CheckChannelI2CConfiguration(const pTCDChannelI2CConfiguration pI2CConfig, std::string &resultHint)
{
	if (CheckError(pI2CConfig))
	{
		return TCD_PARAM_ERROR;
	}

	//Copy configuration
	TCDChannelI2CConfiguration OldI2CConfiguration;
	CopyConfiguration(&OldI2CConfiguration, &m_channelI2CConfig);

	int retVal = 0;
	//Set configuration to test if it's valid.
	if (0 != (retVal = SetChannelI2CConfiguration(pI2CConfig)))
	{
		//Configuration is not valid.
		GetErrorHintString(retVal, resultHint);
	}

	//Former configuration is not well valid? 
	if (0 != SetChannelI2CConfiguration(&OldI2CConfiguration))
	{
		//Former configuration is not valid. Serial number may be the same as the other one.
		return TCD_OTHER_ERROR;
	}

	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::CheckChannelUARTConfiguration(const pTCDChannelUARTConfiguration pUARTConfig, std::string &resultHint)
{
	if (CheckError(pUARTConfig))
	{
		return TCD_PARAM_ERROR;
	}

	//Copy configuration
	TCDChannelUARTConfiguration OldUARTConfiguration;
	CopyConfiguration(&OldUARTConfiguration, &m_channelUARTConfig);

	int retVal = 0;
	//Set configuration to test if it's valid.
	if (0 != (retVal = SetChannelUARTConfiguration(pUARTConfig)))
	{
		//Configuration is not valid.
		GetErrorHintString(retVal, resultHint);
	}

	//Former configuration is not well valid? 
	if (0 != SetChannelUARTConfiguration(&OldUARTConfiguration))
	{
		//Former configuration is not valid. Serial number may be the same as the other one.
		return TCD_OTHER_ERROR;
	}
	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::SaveChannelCommonConfiguration(const pTCDChannelCommonConfiguration pcommonConfig)
{
	if (CheckError(pcommonConfig))
	{
		return TCD_PARAM_ERROR;
	}

	if (0 != SetChannelCommonConfiguration(pcommonConfig))
	{
		return TCD_API_SEQUENCE_ERROR;
	}
	
	CopyConfiguration(&m_channelCommConfig, pcommonConfig);
	
	if (0 !=UpdateConfigurationFile(CONFIGTYPE_COMMON))
	{
		return TCD_FILE_ERROR;
	}

	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::SaveChannelI2CConfiguration(const pTCDChannelI2CConfiguration pI2CConfig)
{
	if (CheckError(pI2CConfig))
	{
		return TCD_PARAM_ERROR;
	}

	if (0 != SetChannelI2CConfiguration(pI2CConfig))
	{
		return TCD_API_SEQUENCE_ERROR;
	}
	CopyConfiguration(&m_channelI2CConfig, pI2CConfig);

	if (0 !=UpdateConfigurationFile(CONFIGTYEP_I2C))
	{
		return TCD_FILE_ERROR;
	}
	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::SaveChannelUARTConfiguration(const pTCDChannelUARTConfiguration pUARTConfig)
{
	if (CheckError(pUARTConfig))
	{
		return TCD_PARAM_ERROR;
	}

	if (0 != SetChannelUARTConfiguration(pUARTConfig))
	{
		return TCD_API_SEQUENCE_ERROR;
	}

	CopyConfiguration(&m_channelUARTConfig, pUARTConfig);

	if (0 !=UpdateConfigurationFile(CONFIGTYPE_UART))
	{
		return TCD_FILE_ERROR;
	}
	return TCD_OK;
}

uint32 TCD_FTDICHIP_ChannelConfiguration::SetChannelFormat(uchar formatByte)
{
	if (0 == (formatByte & AVAILABLE_FORMAT) || !IsSingleBitOnByte(formatByte))
	{
		return TCD_FORMAT_NOT_SUPPORTED;
	}

	m_format = formatByte;

	if (0 !=UpdateConfigurationFile(CONFIGTYPE_FORMAT))
	{
		return TCD_FILE_ERROR;
	}

	return TCD_OK;
}

//Get original configuration.
int TCD_FTDICHIP_ChannelConfiguration::LoadConfigurationFromFile(std::string strPath)
{
	int res = 0;
	fstream fsTmp;
	fsTmp.open(strPath.c_str(), std::ios::in | std::ios::_Nocreate);

	if (!fsTmp.is_open())
	{
		return -1;
	}

	TCDConfigLog::SetPath(strPath);
	if (!TCDConfigLog::Open())
	{
		return -1;
	}

	res = TCDConfigLog::LoadSerialNumber(m_serialNumber);
	if (0 != res)
	{
		TCDConfigLog::Close();
		return -1;
	}

	res |= TCDConfigLog::LoadCurrentFormat(m_serialNumber, &m_format);
	if (0 != res)
	{
		TCDConfigLog::Close();
		return -1;
	}

	res = TCDConfigLog::LoadCommonConfiguration(&m_channelCommConfig, m_serialNumber);
	if (0 != res)
	{
		TCDConfigLog::Close();
		return -1;
	}

	res = TCDConfigLog::LoadI2CConfiguration(&m_channelI2CConfig, m_serialNumber);
	if (0 != res)
	{
		TCDConfigLog::Close();
		return -1;
	}

	res = TCDConfigLog::LoadUARTConfiguration(&m_channelUARTConfig, m_serialNumber);
	if (0 != res)
	{
		TCDConfigLog::Close();
		return -1;
	}

	TCDConfigLog::Close();
	//
	//there is no config file now.
	return 0;
}

int TCD_FTDICHIP_ChannelConfiguration::SaveConfigurationToFile(std::string strPath)
{
	int res = 0;
	std::string strResult;

	res |= CheckChannelCommConfiguration(&m_channelCommConfig, strResult);
	res |= CheckChannelI2CConfiguration(&m_channelI2CConfig, strResult);
	res |= CheckChannelUARTConfiguration(&m_channelUARTConfig, strResult);

	if (0 != res)
	{
		return -1;
	}

	TCDConfigLog::SetPath(strPath);
	if (!TCDConfigLog::Open())
	{
		return -1;
	}

	res = TCDConfigLog::SaveSerialNumber(m_serialNumber);
	if (0 != res)
	{
		TCDConfigLog::Close();
		return -1;
	}

	res |= TCDConfigLog::SaveCurrentFormat(m_serialNumber, m_format);
	if (0 != res)
	{
		TCDConfigLog::Close();
		return -1;
	}

	
	res = TCDConfigLog::SaveCommonConfiguration(&m_channelCommConfig, m_serialNumber);
	if (0 != res)
	{
		TCDConfigLog::Close();
		return -1;
	}

	res = TCDConfigLog::SaveI2CConfiguration(&m_channelI2CConfig, m_serialNumber);
	if (0 != res)
	{
		TCDConfigLog::Close();
		return -1;
	}

	res = TCDConfigLog::SaveUARTConfiguration(&m_channelUARTConfig, m_serialNumber);
	if (0 != res)
	{
		TCDConfigLog::Close();
		return -1;
	}

	TCDConfigLog::Close();
	//
	//there is no config file now.
	return 0;
}

int TCD_FTDICHIP_ChannelConfiguration::UpdateConfigurationFile(int configType)
{
	if (!TCDConfigLog::Open())
	{
		return -1;
	}

	uchar current_format;
	if (0 != TCDConfigLog::LoadCurrentFormat(m_serialNumber, &current_format))
	{
		//A new item need to be created.
		TCDConfigLog::InsertItem(m_serialNumber);
		TCDConfigLog::Close();
		return 0;
	}

	switch (configType)
	{
	case CONFIGTYPE_FORMAT:
		TCDConfigLog::SaveCurrentFormat(m_serialNumber, m_format);
		break;
	case CONFIGTYPE_COMMON:
		TCDConfigLog::SaveCommonConfiguration(&m_channelCommConfig, m_serialNumber);
		break;
	case CONFIGTYEP_I2C:
		TCDConfigLog::SaveI2CConfiguration(&m_channelI2CConfig, m_serialNumber);
		break;
	case CONFIGTYPE_UART:
		TCDConfigLog::SaveUARTConfiguration(&m_channelUARTConfig, m_serialNumber);
		break;
	default:
		return -1;
	}
	TCDConfigLog::Close();
	return 0;
}

int TCD_FTDICHIP_ChannelConfiguration::LoadDefaultConfiguration()
{
	//TODO:Why it's not applicable if const_cast is not used?
	//
	const pTCDChannelCommonConfiguration pconst_Common =const_cast<const pTCDChannelCommonConfiguration>(&default_CommonConfiguration);
	CopyConfiguration(&m_channelCommConfig, pconst_Common);

	const pTCDChannelI2CConfiguration pconst_I2C = const_cast<const pTCDChannelI2CConfiguration>(&default_I2CConfiguration);
	CopyConfiguration(&m_channelI2CConfig, pconst_I2C);

	const pTCDChannelUARTConfiguration pconst_UART = const_cast<const pTCDChannelUARTConfiguration>(&default_UART_Configuration);
	CopyConfiguration(&m_channelUARTConfig, pconst_UART);
	
	m_format = default_format;
	return 0;
}

//May dead functions if configuration file is changed to an unuseful state.
int TCD_FTDICHIP_ChannelConfiguration::SetChannelCommonConfiguration(const pTCDChannelCommonConfiguration pcommonConfig)
{
	//Keep old common configuration if set failed: m_channelCommConfig
	FT_STATUS ftStatus = 0;
	ftStatus = FT_SetUSBParameters(m_tcdHandle, pcommonConfig->USBBuffer, 0);
	if (FT_OK != ftStatus)
	{
		if (0 != SetChannelCommonConfiguration(&m_channelCommConfig))
		{
			SetChannelCommonConfiguration(const_cast<pTCDChannelCommonConfiguration>(&default_CommonConfiguration));
		}
		return USBuffer_ERROR;
	}

	ftStatus = FT_SetTimeouts(m_tcdHandle, pcommonConfig->readtimeouts, pcommonConfig->writetimeouts);
	if (FT_OK != ftStatus)
	{
		if (0 != SetChannelCommonConfiguration(&m_channelCommConfig))
		{
			SetChannelCommonConfiguration(const_cast<pTCDChannelCommonConfiguration>(&default_CommonConfiguration));
		}
		return readwritetimeouts_ERROR;
	}

	ftStatus = FT_SetResetPipeRetryCount(m_tcdHandle, pcommonConfig->retrycount);
	if (FT_OK != ftStatus)
	{
		if (0 != SetChannelCommonConfiguration(&m_channelCommConfig))
		{
			SetChannelCommonConfiguration(const_cast<pTCDChannelCommonConfiguration>(&default_CommonConfiguration));
		}
		return retrycount_ERROR;
	}

	if (pcommonConfig->latencytime < 2)
	{
		if (0 != SetChannelCommonConfiguration(&m_channelCommConfig))
		{
			SetChannelCommonConfiguration(const_cast<pTCDChannelCommonConfiguration>(&default_CommonConfiguration));
		}
		return latencytime_ERROR; 
	}

	ftStatus = FT_SetLatencyTimer(m_tcdHandle, pcommonConfig->latencytime);
	if (FT_OK != ftStatus)
	{
		if (0 != SetChannelCommonConfiguration(&m_channelCommConfig))
		{
			SetChannelCommonConfiguration(const_cast<pTCDChannelCommonConfiguration>(&default_CommonConfiguration));
		}
		return latencytime_ERROR; 
	}

	return 0;
}

int TCD_FTDICHIP_ChannelConfiguration::SetChannelI2CConfiguration(const pTCDChannelI2CConfiguration pI2CConfig)
{
	if (pI2CConfig->clockrate != TCD_I2C_CLOCK_STANDARD_MODE &&
		pI2CConfig->clockrate != TCD_I2C_CLOCK_STANDARD_MODE_3P &&
		pI2CConfig->clockrate != TCD_I2C_CLOCK_FAST_MODE &&
		pI2CConfig->clockrate != TCD_I2C_CLOCK_FAST_MODE_PLUS &&
		pI2CConfig->clockrate != TCD_I2C_CLOCK_HIGH_SPEED_MODE)
	{
		return clockrate_ERROR;
	}
	
	FT_STATUS ftStatus = 0;
	ChannelConfig channel_config;
	channel_config.ClockRate = (I2C_ClockRate_t)pI2CConfig->clockrate;
	channel_config.LatencyTimer = m_channelCommConfig.latencytime;
	channel_config.Options = 0;

	if (false == pI2CConfig->threephaseclock)
	{
		channel_config.Options = I2C_DISABLE_3PHASE_CLOCKING;
	} 

	ftStatus = p_I2C_InitChannel(m_tcdHandle, &channel_config);
	if (FT_OK != ftStatus)
	{
		//TODO this place is not correct.
		if (0 != SetChannelI2CConfiguration(&m_channelI2CConfig))
		{
			SetChannelI2CConfiguration(const_cast<pTCDChannelI2CConfiguration>(&default_I2CConfiguration));
		}
		return clockrate_ERROR;
	}

	return 0;
}

int TCD_FTDICHIP_ChannelConfiguration::SetChannelUARTConfiguration(const pTCDChannelUARTConfiguration pUARTConfig)
{
	//baudrate, stopBits, uParity may not be correct.
	FT_STATUS ftStatus = 0;
	ftStatus = FT_SetBaudRate(m_tcdHandle, pUARTConfig->baudrate);
	if (FT_OK != ftStatus)
	{
		if (0 != SetChannelUARTConfiguration(&m_channelUARTConfig))
		{
			SetChannelUARTConfiguration(const_cast<pTCDChannelUARTConfiguration>(&default_UART_Configuration));
		}
		return baudrate_ERROR;
	}
	
	uchar uWordLength = pUARTConfig->bitsperword;
	uchar uStopBits = pUARTConfig->stopbits;
	uchar uParity = pUARTConfig->parity;
	
	if (uWordLength !=FT_BITS_8 && uWordLength != FT_BITS_7)
	{
		if (0 != SetChannelUARTConfiguration(&m_channelUARTConfig))
		{
			SetChannelUARTConfiguration(const_cast<pTCDChannelUARTConfiguration>(&default_UART_Configuration));
		}
		return bitsperword_ERROR;
	}

	//convert stopbits to FT format

	if (uStopBits != FT_STOP_BITS_1 && uStopBits != FT_STOP_BITS_2)
	{
		if (0 != SetChannelUARTConfiguration(&m_channelUARTConfig))
		{
			SetChannelUARTConfiguration(const_cast<pTCDChannelUARTConfiguration>(&default_UART_Configuration));
		}
		return stopbits_ERROR;
	}

	if (uParity != FT_PARITY_NONE && uParity != FT_PARITY_ODD
		&& uParity != FT_PARITY_EVEN && uParity != FT_PARITY_SPACE)
	{
		if (0 != SetChannelUARTConfiguration(&m_channelUARTConfig))
		{
			SetChannelUARTConfiguration(const_cast<pTCDChannelUARTConfiguration>(&default_UART_Configuration));
		}
		return parity_ERROR;
	}

	ftStatus = FT_SetDataCharacteristics(m_tcdHandle, pUARTConfig->bitsperword, pUARTConfig->stopbits, pUARTConfig->parity);
	if (FT_OK != ftStatus)
	{
		if (0 != SetChannelUARTConfiguration(&m_channelUARTConfig))
		{
			SetChannelUARTConfiguration(const_cast<pTCDChannelUARTConfiguration>(&default_UART_Configuration));
		}
		return bitsperword_ERROR;
	}

	//TODO:
	UCHAR Xon = 0, Xoff = 0;
	USHORT flowcontrol = static_cast<USHORT>(pUARTConfig->flowcontrol);
	ftStatus = FT_SetFlowControl(m_tcdHandle, flowcontrol, Xon, Xoff);
	return 0;
}

int TCD_FTDICHIP_ChannelConfiguration::SetSerialNumber(std::string strSerialNumber)
{
	if (!m_serialNumber.empty() && 0 != m_serialNumber.compare(strSerialNumber))
	{
		//if (!TCDConfigLog::Open())
		//{
		//	return -1;
		//}

		/*TCDConfigLog::DeleteItem(m_serialNumber);
		TCDConfigLog::InsertItem(strSerialNumber);
		TCDConfigLog::Close();*/
	}

	m_serialNumber.assign(strSerialNumber);
	return 0;
}

int TCD_FTDICHIP_ChannelConfiguration::GetAddress(puchar paddr)
{
	*paddr = m_address;
	return 0;
}

static bool CopyConfiguration(pTCDChannelCommonConfiguration pDes, const pTCDChannelCommonConfiguration pSrc)
{
	if (NULL == pSrc || NULL == pDes)
	{
		return false;
	}
	CopyConfiguration(pDes, *pSrc);
	return true;
}

static bool CopyConfiguration(pTCDChannelCommonConfiguration pDes, TCDChannelCommonConfiguration Src)
{
	if (NULL == pDes)
	{
		return false;
	}
	pDes->readtimeouts = Src.readtimeouts;
	pDes->writetimeouts = Src.writetimeouts;
	pDes->latencytime = Src.latencytime;
	pDes->retrycount = Src.retrycount;
	pDes->USBBuffer = Src.USBBuffer;
	return true;
}

static bool CopyConfiguration(pTCDChannelI2CConfiguration pDes, const pTCDChannelI2CConfiguration pSrc)
{
	if (NULL == pSrc || NULL == pDes)
	{
		return false;
	}
	CopyConfiguration(pDes, *pSrc);
	return true;
}

static bool CopyConfiguration(pTCDChannelI2CConfiguration pDes, TCDChannelI2CConfiguration Src)
{
	if (NULL == pDes)
	{
		return false;
	}

	pDes->clockrate = Src.clockrate;
	pDes->threephaseclock = Src.threephaseclock;
	pDes->slaveaddress = Src.slaveaddress;
	return true;
}

static bool CopyConfiguration(pTCDChannelUARTConfiguration pDes, const pTCDChannelUARTConfiguration pSrc)
{
	if (NULL == pSrc || NULL == pDes)
	{
		return false;
	}

	pDes->baudrate = pSrc->baudrate;
	pDes->bitsperword = pSrc->bitsperword;
	pDes->flowcontrol = pSrc->flowcontrol;
	pDes->parity = pSrc->parity;
	pDes->stopbits = pSrc->stopbits;
	return true;
}

static bool CopyConfiguration(pTCDChannelUARTConfiguration pDes, TCDChannelUARTConfiguration Src)
{
	if (NULL == pDes)
	{
		return false;
	}

	pDes->baudrate = Src.baudrate;
	pDes->bitsperword = Src.bitsperword;
	pDes->flowcontrol =Src.flowcontrol;
	pDes->parity = Src.parity;
	pDes->stopbits = Src.stopbits;
	return true;
}


static bool CopyConfigHint(pTCDChannelCommonConfigHint pDes, const pTCDChannelCommonConfigHint pSrc)
{
	if (NULL == pSrc || NULL == pDes)
	{
		return false;
	}
	memcpy(pDes->latencytimeDescription, pSrc->latencytimeDescription, sizeof(pSrc->latencytimeDescription));
	memcpy(pDes->readtimeoutsDescription, pSrc->readtimeoutsDescription, sizeof(pSrc->readtimeoutsDescription));
	memcpy(pDes->retrycountDescription, pSrc->retrycountDescription, sizeof(pSrc->retrycountDescription));
	memcpy(pDes->USBBufferDescription, pSrc->USBBufferDescription, sizeof(pSrc->USBBufferDescription));
	memcpy(pDes->writetimeoutsDescription, pSrc->writetimeoutsDescription, sizeof(pSrc->writetimeoutsDescription));

	return true;
}

static bool CopyConfigHint(pTCDChannelI2CConfigHint pDes, const pTCDChannelI2CConfigHint pSrc)
{
	if (NULL == pSrc || NULL == pDes)
	{
		return false;
	}

	memcpy(pDes->clockrateDescription, pSrc->clockrateDescription, sizeof(pSrc->clockrateDescription));
	memcpy(pDes->clockrateSupported, pSrc->clockrateSupported, sizeof(pSrc->clockrateSupported));
	memcpy(pDes->threephaseclockDescription, pSrc->threephaseclockDescription, sizeof(pSrc->threephaseclockDescription));

	return true;
}

static bool CopyConfigHint(pTCDChannelUARTConfigHint pDes, const pTCDChannelUARTConfigHint pSrc)
{
	if (NULL == pSrc || NULL == pDes)
	{
		return false;
	}

	memcpy(pDes->baudrateDescription, pSrc->baudrateDescription, sizeof(pSrc->baudrateDescription));
	memcpy(pDes->bitsperwordSupported, pSrc->bitsperwordSupported, sizeof(pSrc->bitsperwordSupported));
	memcpy(pDes->flowcontrolSupported, pSrc->flowcontrolSupported, sizeof(pSrc->flowcontrolSupported));
	memcpy(pDes->paritySupported, pSrc->paritySupported, sizeof(pSrc->paritySupported));
	memcpy(pDes->stopbitsSupported, pSrc->stopbitsSupported, sizeof(pSrc->stopbitsSupported));

	return true;
}

static void GetErrorHintString(int erroType, std::string &resultHint)
{
	resultHint.clear();
	int len = sizeof(configStructMap);
	for (int i=0; i != len; ++i)
	{
		if (erroType == configStructMap[i].error_code)
		{
			resultHint.assign(configStructMap[i].strError);
			return;
		}
	}
}

//Check if one bit is '1' in  
static bool IsSingleBitOnByte(uchar oneByte)
{
	bool flag = false;
	uchar multiplication = 1;
	for (int i=0; i != sizeof(uchar)*8; ++i)
	{
		if (oneByte == multiplication)
		{
			flag = true;
			break;
		}
		multiplication *= 2;
	}

	return flag;
}

