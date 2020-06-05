#include "msg_handler.h"
#include "Logger.h"
#include "ArduinoComm.h"

unsigned long msg_handler::free_filter_id = 1;

can_handler::can_handler()
{
	LOGGER.logDebug("HANDLE_CAN", "CAN Handler init");
}

int can_handler::send_payload(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout, unsigned long baud)
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
	this->bs = 0;
	this->st_min = 0;
}

int iso15765_handler::send_payload(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout, unsigned long baud)
{
	LOGGER.logDebug("HANDLE_157_SEND", "Sending %lu messages", *pNumMsgs);
	// Set the arduinos baud rate
	DATA_PAYLOAD p = { 0x01, CMD_BAUD };
	switch (baud) {
	case 83333:
		p.args[0] = 8;
		break;
	case 500000:
		p.args[0] = 50;
		break;
	default:
		LOGGER.logError("HANDLE_157_SEND", "Cannot send at %lu bps over CAN", baud);
		return ERR_NOT_SUPPORTED;
	}
	if (!ArduinoComm::writeData(&p)) {
		return ERR_INVALID_DEVICE_ID;
	}
	for (unsigned long i = 0; i < *pNumMsgs; i++) {
		LOGGER.logDebug("--> Message (Send)", LOGGER.passThruMsg_toString(&pMsg[i]));
		internal_send_payload(&pMsg[i]);
	}
	return 0;
}

int iso15765_handler::read_messages(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout)
{
	return 0;
}

int iso15765_handler::internal_send_payload(PASSTHRU_MSG* m)
{
	if (m->DataSize <= 11) {
		DATA_PAYLOAD p = { 0x00 };
		p.argSize = 12; // 8 bytes + CID (4 bytes)
		p.cmd = CMD_CAN; // Sending a can frame
		memcpy(&p.args[0], &m->Data[0], 4); // Copy CAN ID
		p.args[4] = m->DataSize - 4; // ISO Structure (How many bytes)
		// Copy data
		memcpy(&p.args[5], &m->Data[4], m->DataSize - 4);
		if (ArduinoComm::writeData(&p)) {
			return STATUS_NOERROR;
		}
		else {
			return ERR_INVALID_DEVICE_ID;
		}
	}
	else {
		// TODO ISO WRAPPING
		LOGGER.logError("157_SEND", "Sending payload > 12 bytes not implimented");
		return ERR_BUFFER_FULL;
	}
}

int msg_handler::add_filter(unsigned long FilterType, PASSTHRU_MSG* pMaskMsg, PASSTHRU_MSG* pPatternMsg, PASSTHRU_MSG* pFlowControlMsg, unsigned long* pFilterID)
{
	LOGGER.logInfo("HANDLER_GEN", "Setting up filter. ID: %lu", free_filter_id);
	filter f = filter();
	f.control = pFlowControlMsg;
	f.filterType = FilterType;
	f.mask = pMaskMsg;
	f.pattern = pPatternMsg;
	this->filters.emplace(free_filter_id, f);
	*pFilterID = free_filter_id;
	free_filter_id++;
	return STATUS_NOERROR;
}

int msg_handler::remove_filter(unsigned long filterID)
{
	if (filters.find(filterID) == filters.end()) {
		return ERR_INVALID_FILTER_ID;
	}
	filters.erase(filterID);
	return STATUS_NOERROR;
}
