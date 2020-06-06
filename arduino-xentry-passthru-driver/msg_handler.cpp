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

void can_handler::processPayload(DATA_PAYLOAD* p)
{
}

iso15765_handler::iso15765_handler()
{
	LOGGER.logDebug("HANDLE_157", "ISO15765 Handler init");
	this->bs = 0;
	this->st_min = 0;
	this->temp_ms = { 0x00 };
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
		send_queue.push(pMsg[i]); // Push to the queue
		internal_send_payload();
	}
	return 0;
}

void iso15765_handler::processPayload(DATA_PAYLOAD* p)
{
	int match_count = 0;
	bool matched = false;
	if (filters.size() == 0) { return; } // Return if we have no filters!
	for (auto& f : filters) {
		PASSTHRU_MSG* mask = &f.second.mask;
		// Block filter
		if (f.second.filterType == BLOCK_FILTER) {
			for (int i = 0; i < mask->DataSize; i++) {
				// Match so this is not for us!
				if ((p->args[i] & mask->Data[i]) != f.second.pattern.Data[i]) {
					match_count++;
				}
			}
		}
		// Pass filter (ISO is also pass)
		else {
			for (int i = 0; i < mask->DataSize; i++) {
				// Does not match so this is not for us!
				if ((p->args[i] & mask->Data[i]) == f.second.pattern.Data[i]) {
					match_count++;
				}
			}
		}
		if (match_count == f.second.pattern.DataSize) {
			matched = true;

			// Send flow control now!
			if (f.second.filterType == FLOW_CONTROL_FILTER && p->args[4] == 0x10) {
				LOGGER.logDebug("ISO15765_PROC", "Sending flow control back to ECU");
				DATA_PAYLOAD m = {0x00};
				m.cmd = CMD_CAN;
				m.argSize = 12;
				memcpy(&m.args[0], f.second.control.Data, 4); // Copy response CID
				m.args[4] = 0x30;
				m.args[5] = (uint8_t)this->bs;
				m.args[6] = (uint8_t)this->st_min;
				ArduinoComm::writeData(&m);
			}
			break; 
		}
	}
	if (!matched) { return; }
	// So now we know this is for us!
	LOGGER.logInfo("ISO15765_PROC", "Found payload match for payload: " + LOGGER.bytesToString(p->argSize, p->args));
	PASSTHRU_MSG m = { 0x00 };
	DATA_PAYLOAD s = { 0x00 };
	m.ProtocolID = ISO15765;
	switch (p->args[4] & 0xF0) {
	case 0x00:
		LOGGER.logDebug("ISO15765_PROC", "Adding complete message to queue");
		m.DataSize = 4 + p->args[4];
		memcpy(&m.Data[0], &p->args[0], 4); // Copy CID
		memcpy(&m.Data[4], &p->args[5], p->args[5]); // Copy ISO Contents
		recv_queue.push(m);
		return;
	case 0x10:
		num_bytes = p->args[5];
		LOGGER.logDebug("ISO15765_PROC", "Got head of multi-frame. Total size should be %d bytes", num_bytes);
		memcpy(&rxBuffer[0], &p->args[6], 6);
		rxBufferPos += 6; // 12 - 4 (CID) - 2 (0x10, Size)


		// Send TX OK notification
		/*
		m.RxStatus = ISO15765_FIRST_FRAME;
		send_buffer[send_buffer_size] = m;
		send_buffer_size++;
		*/
		return;
	case 0x20:
		if (rxBufferPos + 7 >= num_bytes) {
			LOGGER.logDebug("ISO15765_PROC", "Got tail of multi-frame");
			memcpy(&rxBuffer[rxBufferPos], &p->args[5], num_bytes - rxBufferPos);
			memcpy(&m.Data[0], &p->args[0], 4); // Copy CID
			memcpy(&m.Data[4], &rxBuffer[0], num_bytes);
			m.DataSize = 4 + num_bytes;
			//m.RxStatus = TX_MSG_TYPE;
			recv_queue.push(m);
			rxBufferPos = 0;
			num_bytes = 0;
			sending = false;
		}
		else {
			LOGGER.logDebug("ISO15765_PROC", "Got mid of multi-frame");
			memcpy(&rxBuffer[rxBufferPos], &p->args[5], 7);
			rxBufferPos += 7;
		}
		return;
	case 0x30:
		if (num_bytes_to_send > 0) {
			int count = 0;
			while (num_bytes_sent < num_bytes_to_send && count < this->bs) {
				s.cmd = CMD_CAN;
				s.argSize = 12;
				memcpy(&s.args[0], &temp_ms.Data[0], 4); // Copy CID
				s.args[4] = frame_num;
				frame_num++;
				memcpy(&s.args[5], &temp_ms.Data[num_bytes_sent+4], 7); // +4 so we skip the CID
				num_bytes_sent += 7;
				ArduinoComm::writeData(&s);
				count++;
				// Reset PCI on overflow
				if (frame_num == 0x30) {
					frame_num = 0x20;
				}
			}
			if (count == this->bs) {
				LOGGER.logDebug("ISO15765_PROC", "Max block size reached! - %d bytes left", num_bytes_to_send - num_bytes_sent);
				//frame_num = 0x21;
			}
			if (num_bytes_sent >= num_bytes_to_send) {
				LOGGER.logDebug("ISO15765_PROC", "Completed sending entire message!");
				num_bytes_to_send = 0;
				// Notify that RX is done!

				m.RxStatus = TX_MSG_TYPE;
				m.DataSize = 4;
				memcpy(&m.Data[0], &p->args[0], 4); // Copy CID;
				recv_queue.push(m);
				send_queue.pop(); // Finished sending
			}
		}
		return;
	default:
		LOGGER.logError("ISO15765_PROC", "Invalid ISO15765 Response byte: %02X", p->args[4]);
		return;
	}
	// We should send something!
	if (!sending && send_queue.size() > 0 && !sending) {
		internal_send_payload();
	}
}

int iso15765_handler::internal_send_payload()
{
	PASSTHRU_MSG* m = &send_queue.front();
	if (m->DataSize <= 11) {
		DATA_PAYLOAD p = { 0x00 };
		p.argSize = 12; // 8 bytes + CID (4 bytes)
		p.cmd = CMD_CAN; // Sending a can frame
		memcpy(&p.args[0], &m->Data[0], 4); // Copy CAN ID
		p.args[4] = uint8_t(m->DataSize - 4); // ISO Structure (How many bytes)
		// Copy data
		memcpy(&p.args[5], &m->Data[4], m->DataSize - 4);
		send_queue.pop();
		sending = false;
		if (ArduinoComm::writeData(&p)) {
			return STATUS_NOERROR;
		}
		else {
			return ERR_INVALID_DEVICE_ID;
		}
	}
	else {
		sending = true;
		num_bytes_to_send = m->DataSize - 4; // Excludes CID bytes
		LOGGER.logError("157_SEND", "Sending first frame of multi part message. Total size is %lu bytes", num_bytes_to_send);
		DATA_PAYLOAD p = { 0x00 };
		memcpy(&temp_ms, m, sizeof(PASSTHRU_MSG)); // Copy over passthru_msg so we know the rest of the data

		// Construct the first CAN Frame (has 0x10 followed by the total payload size)
		p.cmd = CMD_CAN;
		p.argSize = 12;
		memcpy(&p.args[0], &m->Data[0], 4); // Copy CAN ID
		p.args[4] = 0x10;
		p.args[5] = num_bytes_to_send;
		memcpy(&p.args[6], &m->Data[4], 6); // Max allowed for first frame
		num_bytes_sent = 6;
		frame_num = 0x21;
		ArduinoComm::writeData(&p);
		return STATUS_NOERROR;
	}
}

int msg_handler::read_messages(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout)
{
	if (Timeout != 0) {
		std::this_thread::sleep_for(std::chrono::milliseconds(Timeout));
	}
	if (recv_queue.size() == 0) {
		return ERR_BUFFER_EMPTY;
	}
	LOGGER.logDebug("HANDLER_READ", "Want %lu, copying %lu messages back to Application", *pNumMsgs, min(*pNumMsgs, recv_queue.size()));
	for (unsigned long i = 0; i < min(*pNumMsgs, recv_queue.size()); i++) {
		memcpy(&pMsg[i], &recv_queue.front(), sizeof(PASSTHRU_MSG));
		LOGGER.logDebug("(Recv)-->", "DATA: "+LOGGER.bytesToString(pMsg[i].DataSize, pMsg[i].Data));
		recv_queue.pop();
	}
	return STATUS_NOERROR;
}

int msg_handler::add_filter(unsigned long FilterType, PASSTHRU_MSG* pMaskMsg, PASSTHRU_MSG* pPatternMsg, PASSTHRU_MSG* pFlowControlMsg, unsigned long* pFilterID)
{
	LOGGER.logInfo("HANDLER_GEN", "Setting up filter. ID: %lu", free_filter_id);
	switch (FilterType) {
	case PASS_FILTER:
		LOGGER.logDebug("HANDLER_GEN", "Filter is PASS_FILTER");
		break;
	case BLOCK_FILTER:
		LOGGER.logInfo("HANDLER_GEN", "Filter is BLOCK_FILTER");
		break;
	case FLOW_CONTROL_FILTER:
		LOGGER.logInfo("HANDLER_GEN", "Filter is FLOW_CONTROL");
		break;
	default:
		LOGGER.logError("HANDLER_GEN", "Invalid filter type %lu", FilterType);
		return ERR_INVALID_FLAGS;
	}
	filter f = filter();
	memcpy(&f.control, pFlowControlMsg, sizeof(PASSTHRU_MSG));
	memcpy(&f.mask, pMaskMsg, sizeof(PASSTHRU_MSG));
	memcpy(&f.pattern, pPatternMsg, sizeof(PASSTHRU_MSG));
	f.filterType = FilterType;
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
