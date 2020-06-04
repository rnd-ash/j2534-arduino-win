//#ifndef XENTRY_COMM_H_
//#define XENTRY_COMM_H_
#include "pch.h"
#include <Windows.h>

namespace XentryComm {

	bool CreateCommThread();
	void CloseCommThread();
	bool CreateEvents();
	void CloseHandles();
	int WaitUntilReady(const char* deviceName, unsigned long timeout);
};

//#endif

