#pragma comment(lib, "tinyxml.lib")
#include "XMLParser.h"
//#include "tinystr.h"
#include "tinyxml.h"

DeviceDriver_XMLParser::DeviceDriver_XMLParser()
{
	m_document = NULL;
}

DeviceDriver_XMLParser::~DeviceDriver_XMLParser()
{
	delete m_document;
}

int DeviceDriver_XMLParser::LoadXML(const std::string xmlPath)
{
	reset();
	m_document = new TiXmlDocument(xmlPath.c_str());
	m_document->LoadFile();
	return 0;
}

int DeviceDriver_XMLParser::ListFunctions(std::vector<std::string> &funcList)
{
	funcList.clear();
	TiXmlElement *RootElement = m_document->RootElement();
	TiXmlElement *FirstFunction = RootElement->FirstChildElement();
	while(NULL != FirstFunction)
	{
		TiXmlElement *Name = FirstFunction->FirstChildElement();
		funcList.push_back(Name->FirstChild()->Value());
		FirstFunction = FirstFunction->NextSiblingElement();
	}

	return 0;
}

int DeviceDriver_XMLParser::GetFunctionDetail(const std::string name, std::string &details)
{
	enum {
		EQUAL = 0,
	};

	bool flagFound = false;
	details.clear();
	TiXmlElement *RootElement = m_document->RootElement();
	TiXmlElement *FirstFunction = RootElement->FirstChildElement();
	while(NULL != FirstFunction)
	{
		TiXmlElement *Name = FirstFunction->FirstChildElement();
		if (EQUAL == name.compare(Name->FirstChild()->Value()))
		{
			TiXmlElement *Usage = Name->NextSiblingElement();
			details.assign(Usage->FirstChild()->Value());
			flagFound = true;
			break;
		}
		FirstFunction = FirstFunction->NextSiblingElement();
	}

	if (false == flagFound)
	{
		return -1;
	}
	return 0;
}

int DeviceDriver_XMLParser::reset()
{
	delete m_document;
	m_document = NULL;
	return 0;
}