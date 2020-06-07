#include "pch.h"
#include "Logger.h"
#include "XentryComm.h"
#include "ArduinoComm.h"
#include "device.h"

namespace XentryComm{

	HANDLE thread = NULL; // Comm thread
	HANDLE askInitEvent; // Handle for if other threads want to init
	HANDLE commEvent; // Handle for event from arduino
	HANDLE exitEvent; // Handle for exiting / closing thread
	HANDLE closedEvent; // Handle for when thread is closed
	HANDLE events[20];
	int eventCount = 0;


	void CloseHandles() {
		CloseHandle(askInitEvent);
		CloseHandle(exitEvent);
		CloseHandle(commEvent);
		CloseHandle(closedEvent);
	}

	int WaitUntilReady(const char* deviceName, unsigned long timeout) {
		LOGGER.logInfo("XentryComm::Wait", "Waiting for Arduino to be ready");
		if (ArduinoComm::isConnected()) {
			return 0;
		} else {
			if (ArduinoComm::OpenPort()) {
				return 0;
			}
		}
		return 1;
	}

	void CloseCommThread() {
		LOGGER.logInfo("XentryComm::CloseCommThread", "Closing comm thread");
		SetEvent(exitEvent);
		WaitForSingleObject(closedEvent, 5000); // Wait for 5 seconds for the thread to terminate
		CloseHandles();
		CloseHandle(thread);
	}

	bool CreateEvents() {
		askInitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (askInitEvent == NULL) {
			LOGGER.logWarn("XentryComm::CreateEvents", "Cannot create init event!");
			return false;
		}

		exitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (exitEvent == NULL) {
			LOGGER.logWarn("XentryComm::CreateEvents", "Cannot create exit event!");
			return false;
		}
		closedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (closedEvent == NULL) {
			LOGGER.logWarn("XentryComm::CreateEvents", "Cannot create closed event!");
			return false;
		}
		commEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (commEvent == NULL) {
			LOGGER.logWarn("XentryComm::CreateEvents", "Cannot create comm event!");
			return false;
		}

		events[0] = askInitEvent;
		events[1] = commEvent;
		events[2] = exitEvent;
		eventCount = 3;
		return true;
	}

	bool waitForEvents() {
		DWORD ret;
		ret = WaitForMultipleObjects(eventCount, events, false, INFINITE);
		// Event[0] = Init
		// Event[1] = Comm
		// Event[2] = Exit
		if (ret == (WAIT_OBJECT_0 + 0)) {
			LOGGER.logInfo("XentryComm::waitForEvents", "Init event handled");
			// TODO Handle Init event
		} else if (ret == (WAIT_OBJECT_0 + 1)) {
			LOGGER.logInfo("XentryComm::waitForEvents", "Communication event handled");
			// TODO Handle Communication event
		} else if (ret == (WAIT_OBJECT_0 + 2)) {
			LOGGER.logInfo("XentryComm::waitForEvents", "Exit event handled");
			// TODO Handle exit event
			return false;
		} else {
			LOGGER.logInfo("XentryComm::waitForEvents", "Unkonwn handle!");
			return false;
		}
		return true;
	}
	
	DATA_PAYLOAD d = { 0x00 };
	DWORD WINAPI CommLoop() {
		LOGGER.logInfo("XentryComm::CommLoop", "Starting comm loop");
		while (true) {
			if (ArduinoComm::readPayload(&d)) {
				if (d.cmd == CMD_CAN) {
					dev_map.processPayload(&d);
				}
			}
			dev_map.update_channels();
		}
		return 1;
	}

	DWORD WINAPI startComm(LPVOID lpParam) {
		LOGGER.logInfo("XentryComm::startComm", "started comms!");
		CommLoop(); // This function will stop executing anything below this until we are done
		// TODO Handle arduino upon exit
		LOGGER.logInfo("XentryComm::startComm", "Exiting!");
		SetEvent(closedEvent);
		return 0;
	}

	bool CreateCommThread() {
		// Check if thread is already running
		if (thread == NULL) {
			LOGGER.logInfo("XentryComm::CreateCommThread", "Creating events for thread");
			if (!CreateEvents()) {
				LOGGER.logError("XentryComm::CreateCommThread", "Failed to create events!");
				return false;
			}
			LOGGER.logInfo("XentryComm::CreateCommThread", "Creating thread");
			thread = CreateThread(NULL, 0, startComm, NULL, 0, NULL);
			if (thread == NULL) {
				LOGGER.logError("XentryComm::CreateCommThread", "Thread could not be created!");
				return false;
			}
			LOGGER.logInfo("XentryComm::CreateCommThread", "Thread created!");
		}
		if (WaitUntilReady("", 3000) != 0) {
			LOGGER.logInfo("XentryComm::CreateCommThread", "Arduino could not be created!");
			return false;
		}
		return true;
	}
}