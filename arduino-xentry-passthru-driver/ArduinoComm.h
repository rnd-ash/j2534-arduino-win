#pragma once

#ifndef ARDUINO_COMM_H_
#define ARDUINO_COMM_H_

#include <mutex>
#include <Windows.h>

#define CMD_LOG 0x00
#define CMD_BAUD 0x01
#define CMD_SEND 0x02
#define CMD_CAN 0x03

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

