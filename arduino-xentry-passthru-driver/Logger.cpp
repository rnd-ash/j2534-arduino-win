#include "Logger.h"
#include <mutex>

std::mutex mutex;
void Logger::logInfo(std::string method, std::string message) {
	writeToFile("[INFO ] " + method + " - " + message);
}

void Logger::logWarn(std::string method, std::string message)
{
	writeToFile("[WARN ] " + method + " - " + message);
}

void Logger::logError(std::string method, std::string message)
{
	writeToFile("[ERROR] " + method + " - " + message);
}

void Logger::logDebug(std::string method, std::string message) 
{
	writeToFile("[DEBUG] " + method + " - " + message);
}

void Logger::logInfo(std::string method, const char* fmt, ...) {
	va_list fmtargs;
	va_start(fmtargs, fmt);
	writeToFile("[INFO ] " + method + " - " + argFormatToString(fmt, &fmtargs));
	va_end(fmtargs);
}

void Logger::logWarn(std::string method, const char* fmt, ...) {
	va_list fmtargs;
	va_start(fmtargs, fmt);
	writeToFile("[WARN ] " + method + " - " + argFormatToString(fmt, &fmtargs));
	va_end(fmtargs);
}

void Logger::logError(std::string method, const char* fmt, ...) {
	va_list fmtargs;
	va_start(fmtargs, fmt);
	writeToFile("[ERROR] " + method + " - " + argFormatToString(fmt, &fmtargs));
	va_end(fmtargs);
}

void Logger::logDebug(std::string method, const char* fmt, ...) {
	va_list fmtargs;
	va_start(fmtargs, fmt);
	writeToFile("[DEBUG] " + method + " - " + argFormatToString(fmt, &fmtargs));
	va_end(fmtargs);
}

std::string Logger::payloadToString(DATA_PAYLOAD* p) {
	std::string x = "";
	char buf[4] = { 0x00 };
	for (int i = 0; i < p->argSize; i++) {
		sprintf(buf, "%02X ", p->args[i]);
		x += buf;
	}
	return x;
}

std::string Logger::bytesToString(int size,  unsigned char* bytes)
{
	std::string ret = "";

	char buf[4] = { 0x00 };
	for (int i = 0; i < size; i++) {
		sprintf_s(buf, "%02X ", bytes[i]);
		ret += buf;
	}
	return ret;
}

std::string Logger::argFormatToString(const char* fmt, va_list* args) {
	char buffer[4096] = { 0x00 };
	int rc = vsnprintf_s(buffer, sizeof(buffer), fmt, *args);
	return std::string(buffer);
}

void Logger::writeToFile(std::string message) {
	char time[64] = { 0x00 };
	SYSTEMTIME st;
	GetSystemTime(&st);
	sprintf_s(time, "[%02d:%02d:%02d.%3d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	std::ofstream handle;
	mutex.lock();
	try {
		handle.open(LOG_FILE, std::ios_base::app);
		handle << time << message << "\n" << std::flush;
		handle.close();
#if _DEBUG
		std::cout << message << "\n";
#endif
	}
	catch (std::ofstream::failure e) {
		//TODO handle error
	}
	mutex.unlock();
}

Logger LOGGER = Logger();