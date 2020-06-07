#include "ArduinoComm.h"
#include "pch.h"
#include "Logger.h"


namespace ArduinoComm {
	HANDLE handler;
	bool connected = false;

	std::mutex mutex;
	int pos = 0;
	bool inPayload = false;
	COMSTAT com;
	DWORD errors;
	bool OpenPort() {
		mutex.lock();
		handler = CreateFile(L"COM8", GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (handler == INVALID_HANDLE_VALUE) {
			LOGGER.logError("ARDUINO", "Cannot create handler");
			mutex.unlock();
			return false;
		}

		DCB params = { 0x00 };
		if (!GetCommState(handler, &params)) {
			LOGGER.logError("ARDUINO", "Cannot read comm states");
			mutex.unlock();
			return false;
		}

		params.BaudRate = CBR_115200;
		params.ByteSize = 8;
		params.StopBits = ONESTOPBIT;
		params.Parity = NOPARITY;
		params.fDtrControl = DTR_CONTROL_DISABLE;

		if (!SetCommState(handler, &params)) {
			LOGGER.logError("ARDUINO", "Cannot set comm states");
			mutex.unlock();
			return false;
		}

		PurgeComm(handler, PURGE_RXCLEAR | PURGE_TXCLEAR);
		connected = true;
		mutex.unlock();
		return true;
	}

	void ClosePort() {
		mutex.lock();
		CloseHandle(handler);
		mutex.unlock();
		connected = false;
	}

	bool writeData(DATA_PAYLOAD* f) {
		// Result
		DWORD written = 0;
		// Too big for 1 payload for arduino - TODO - split payloads
		mutex.lock();
		f->argSize++;
		if (!WriteFile(handler, f, f->argSize+1, NULL, NULL)) {
			DWORD error = GetLastError();
			LOGGER.logWarn("ARDUINO", "Error writing data! Code %d", (int)error);
			mutex.unlock();
			f->argSize--;
			return false;
		}
		mutex.unlock();
		return true;
	}

	bool readPayload(DATA_PAYLOAD* f) {
		mutex.lock();
		memset(f, 0x00, sizeof(struct DATA_PAYLOAD));
		ClearCommError(handler, &errors, &com);
		if (com.cbInQue >= 1) {
			uint8_t len;
			ReadFile(handler, &len, 1, NULL, NULL);
			ReadFile(handler, &f->cmd, len, NULL, NULL);
			f->argSize = len - 1;
			mutex.unlock();
			if (f->cmd == CMD_LOG) {
				LOGGER.logInfo("ARDUINO LOG", "%s", f->args);
				return false;
			}
			return true;
		}
		mutex.unlock();
		return false;
	}

	bool isConnected() {
		return connected;
	}
}
