#pragma once

#ifndef ARDUINO_COMM_H_
#define ARDUINO_COMM_H_

#include <mutex>
#include <Windows.h>

#define CMD_LOG 0x00

struct DATA_PAYLOAD {
	uint8_t argSize;
	uint8_t cmd;
	uint8_t args[254];
};


namespace ArduinoComm
{
	bool OpenPort();
	void ClosePort();

	bool writeData(DATA_PAYLOAD* f);
	bool readPayload(DATA_PAYLOAD *f);
	bool isConnected();
};

#endif

