#pragma once

#ifndef LOGGER_H_
#define LOGGER_H_

#include <fstream>
#include <iostream>
#include <sstream>
#include <Windows.h>
#include "j2534_v0404.h"
#include "ArduinoComm.h"

#define LOG_FILE "c:\\arduino_passthru\\activity.log"

class Logger {
public:
	void logInfo(std::string method, std::string message);
	void logWarn(std::string method, std::string message);
	void logDebug(std::string method, std::string message);
	void logError(std::string method, std::string message);
	void logInfo(std::string method, const char* fmt, ...);
	void logWarn(std::string method, const char* fmt, ...);
	void logError(std::string method, const char* fmt, ...);
	void logDebug(std::string method, const char* fmt, ...);
	std::string payloadToString(DATA_PAYLOAD *p);
	std::string bytesToString(int size, unsigned char* bytes);
private:
	std::string argFormatToString(const char* fmt, va_list* args);
	void writeToFile(std::string message);
};

extern Logger LOGGER;

#endif