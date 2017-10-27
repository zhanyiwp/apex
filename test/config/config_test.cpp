#include <iostream>
#include "config.h"


using namespace std;
using namespace Config;

int main(int argc, char const *argv[])
{
	/*
	Config config;
	int ret = config.InitConfig("./text.ini");
	cout << "ret:" << ret << endl;
	cout << "hello.name:" << config.FindValue("hello", "name") << endl;
	cout << "hello.age:" << config.FindValue("hello", "age", "19") << endl;
	cout << "pet.name:" << config.FindValue("pet", "name", "") << endl;
	cout << "pet.animal:" << config.FindValue("pet", "animal", "") << endl;

	cout << "not find:" << config.FindValue("xx", "xxx", "not find!") << endl;
	*/
	string sOut;
	bool bRet = GetString("./text.ini", "hello", "name", "", sOut);
	cout << "bRet:" << bRet << " hello.name:" << sOut << endl;

	bRet = GetString("./text.ini", "", "petname", "", sOut);
	cout << "bRet:" << bRet << " name:" << sOut << endl;

	bRet = GetString("./text.ini", "", "\"  hello  \"", "", sOut);
	cout << "bRet:" << bRet << "  hello  :" << sOut << endl; 
	
	uint8_t uib;
	bRet = GetUInt8("./text.ini", "", "uint8_1", 0, uib);
	cout << "bRet:" << bRet << " uint8_1:" << (int)uib << endl;

	bRet = GetUInt8("./text.ini", "", "uint8_2", 0, uib);
	cout << "bRet:" << bRet << " uint8_2:" << (int)uib << endl; 

	int8_t ib;
	bRet = GetInt8("./text.ini", "", "int8_1", 0, ib);
	cout << "bRet:" << bRet << " int8_1:" << (int)ib << endl;

	bRet = GetInt8("./text.ini", "", "int8_2", 0, ib);
	cout << "bRet:" << bRet << " int8_2:" << (int)ib << endl;

	bRet = GetInt8("./text.ini", "", "int8_3", 0, ib);
	cout << "bRet:" << bRet << " int8_3:" << (int)ib << endl;


	return 0;
}