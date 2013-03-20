//!@file XMLParser.h
//!@brief parse XML file with engaged format
//<?xml version="1.0"?>
//<!-- Ignorance.-->
//<functionlist><!-- This is root .-->
//		<function ID="1">
//		<name>TestFun1</name>
//		<usage>This is Test1 Help String</usage>
//		</function>
//		<function ID="2">
//		<name>TestFunc2</name>
//		<usage>This is Test2 Help String</usage>
//		</function>
//		<!-- More functions are needed here.-->
//</functionlist>
#include <string>
#include <vector>
class TiXmlDocument;

class DeviceDriver_XMLParser
{
public:
	DeviceDriver_XMLParser();
	~DeviceDriver_XMLParser();
	int LoadXML(const std::string xmlPath);
	int ListFunctions(std::vector<std::string> &funcList);
	int GetFunctionDetail(const std::string name, std::string &details);

protected:
	int reset();
private:
	//I declare them but i don't implement them.
	DeviceDriver_XMLParser(const DeviceDriver_XMLParser &other);
	DeviceDriver_XMLParser & DeviceDriver_XMLParser::operator =(const DeviceDriver_XMLParser &other);
	
	TiXmlDocument* m_document;
};