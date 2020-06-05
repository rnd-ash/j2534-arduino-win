#include "msg_handler.h"
#include "Logger.h"

can_handler::can_handler()
{
	LOGGER.logDebug("HANDLE_CAN", "CAN Handler init");
}

int can_handler::send_payload(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout)
{
	return 0;
}

int can_handler::read_messages(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout)
{
	return 0;
}

iso15765_handler::iso15765_handler()
{
	LOGGER.logDebug("HANDLE_157", "ISO15765 Handler init");
}

int iso15765_handler::send_payload(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout)
{
	LOGGER.logDebug("HANDLE_157_SEND", "Sending %lu messages", *pNumMsgs);
	for (unsigned long i = 0; i < *pNumMsgs; i++) {
		LOGGER.logDebug("--> Message (Send)", LOGGER.passThruMsg_toString(&pMsg[i]));
	}
	return 0;
}

int iso15765_handler::read_messages(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout)
{
	return 0;
}
