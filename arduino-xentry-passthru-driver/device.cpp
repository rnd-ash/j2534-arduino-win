#include "device.h"
#include "Logger.h"

unsigned long device_table::free_id = 1;
unsigned long device_table::free_chan_id = 1;

device_table::device_table()
{
	this->device_list = std::map<unsigned long, device>();
}

void device_table::update_channels()
{
	for (auto& channel : channels) {
		channel.second.update_channel();
	}
}

int device_table::remove_device(unsigned long id)
{
	if (device_list.find(id) == device_list.end()) {
		LOGGER.logError("TABLE_REMOVE", "Cannot remove device with ID %lu, does not exist!", id);
		return ERR_INVALID_DEVICE_ID;
	}
	return STATUS_NOERROR;
}

int device_table::add_device(unsigned long id)
{
	if (device_list.find(id) != device_list.end()) {
		LOGGER.logError("TABLE_ADD", "Cannot add device with ID %lu, already exists!", id);
		return ERR_DEVICE_IN_USE;
	}
	device dev = device(id);
	device_list.emplace(id, dev);
	return STATUS_NOERROR;
}

int device_table::add_channel(unsigned long deviceID, unsigned long protocolID, unsigned long Flags, unsigned long baudRate, unsigned long* pChannelID)
{
	if (device_list.find(deviceID) == device_list.end()) {
		LOGGER.logError("TABLE_CHAN_ADD", "Cannot create channel for device %lu. Device does not exist", deviceID);
		return ERR_INVALID_DEVICE_ID;
	}
	*pChannelID = free_chan_id;
	free_chan_id++;
	device_channel chan = device_channel(deviceID);
	chan.setFlags(Flags);
	chan.setProtocol(protocolID);
	chan.setBaudRate(baudRate);
	channels.emplace(*pChannelID, chan);
	return STATUS_NOERROR;
}

int device_table::remove_channel(unsigned long chanID)
{
	if (channels.find(chanID) == channels.end()) {
		LOGGER.logError("TABLE_CHAN_REMOVE", "Cannot remove Channel with ID %lu. Does not exist", chanID);
	}
	channels.at(chanID).closeChannel();
	channels.erase(chanID);
	return STATUS_NOERROR;
}

device* device_table::getDev(unsigned long id)
{
	try {
		return &this->device_list.at(id);
	}
	catch (std::exception e) {
		return nullptr;
	}
}

device_channel* device_table::getChannel(unsigned long id)
{
	try {
		return &this->channels.at(id);
	}
	catch (std::exception e) {
		return nullptr;
	}
}

void device_table::processPayload(DATA_PAYLOAD* msg)
{
	for (auto& chan: channels) {
		chan.second.recvPayload(msg);
	}
}

device_table dev_map;

device::device(unsigned long id)
{
	LOGGER.logDebug("DEV_ADD", "Creating device with ID %lu", id);
	this->id = id;
}

device_channel::device_channel(unsigned long devID)
{
	LOGGER.logInfo("CHAN_INIT", "Setting up new channel for device %lu", devID);
	this->deviceID = devID;
	this->baud = 0;
	this->flags = 0;
	this->protocol = 0xFF;
	this->handler = nullptr;
}

void device_channel::setFlags(unsigned long flags)
{
	LOGGER.logInfo("CHAN_FLGS", "Setting channel flags to %lu", flags);
	this->flags = flags;
}

void device_channel::setProtocol(unsigned long protocol)
{
	LOGGER.logDebug("CHAN_PROT", "Setting channel protocol to %lu", protocol);
	this->protocol = protocol;
	if (this->handler != nullptr) {
		LOGGER.logDebug("CHAN_PROT", "Handler is being replaced, freeing old one");
		free(this->handler);
	}

	switch (protocol)
	{
	case CAN:
		this->handler = new can_handler();
		break;
	case ISO15765:
		this->handler = new iso15765_handler();
		break;
	default:
		LOGGER.logError("CHAN_PROT", "Protocol ID %lu has no associated handler", protocol);
		break;
	}
}

void device_channel::setBaudRate(unsigned long baud)
{
	LOGGER.logDebug("CHAN_BAUD", "Setting channel baudrate to %lu bps", baud);
	this->baud = baud;
}

void device_channel::closeChannel()
{
	LOGGER.logDebug("CHAN_CLOSE", "Closing channel");
}

int device_channel::send_message(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout)
{
	return handler->send_payload(pMsg, pNumMsgs, Timeout, baud);
}

int device_channel::handleIOCTL(unsigned long IoctlID, void* pInput, void* pOutput)
{
	SCONFIG_LIST* input = (SCONFIG_LIST*)pInput;
	switch (IoctlID)
	{
	case GET_CONFIG:
		for (unsigned long i = 0; i < input->NumOfParams; i++) {
			getConfig(&input->ConfigPtr[i]);
		}
		break;
	case SET_CONFIG:
		for (unsigned long i = 0; i < input->NumOfParams; i++) {
			setConfig(&input->ConfigPtr[i]);
		}
		break;
	default:
		LOGGER.logError("CHAN_IOCTL", "Cannot handle IOCTL ID %lu", IoctlID);
		return ERR_INVALID_IOCTL_ID;
	}

	return STATUS_NOERROR;
}

int device_channel::add_filter(unsigned long FilterType, PASSTHRU_MSG* pMaskMsg, PASSTHRU_MSG* pPatternMsg, PASSTHRU_MSG* pFlowControlMsg, unsigned long* pFilterID)
{
	return handler->add_filter(FilterType, pMaskMsg, pPatternMsg, pFlowControlMsg, pFilterID);
}

int device_channel::rem_filter(unsigned long filterID)
{
	LOGGER.logInfo("CHAN_RFILT", "Removing filter with ID %lu", filterID);
	return handler->remove_filter(filterID);
}

void device_channel::recvPayload(DATA_PAYLOAD* p)
{
	if (handler != nullptr) {
		handler->processPayload(p);
	}
}

int device_channel::read_messages(PASSTHRU_MSG* pMsg, unsigned long* pNumMsgs, unsigned long Timeout)
{
	return handler->read_messages(pMsg, pNumMsgs, Timeout);
}

void device_channel::update_channel()
{
	if (handler != nullptr) {
		handler->update();
	}
}

void device_channel::getConfig(SCONFIG* c)
{
}

void device_channel::setConfig(SCONFIG* c)
{
	// Read from sconfig pointer
	switch (c->Parameter) {
	case ISO15765_BS:
		if (protocol == ISO15765) {
			LOGGER.logDebug("CHAN_GCFG", "Set ISO15765 block size to %lu frames", c->Value);
			dynamic_cast<iso15765_handler*>(handler)->bs = c->Value;
		}
		break;
	case ISO15765_STMIN:
		if (protocol == ISO15765) {
			LOGGER.logDebug("CHAN_GCFG", "Set ISO15765 Min Seperation time to %lu ms", c->Value);
			dynamic_cast<iso15765_handler*>(handler)->st_min = c->Value;
		}
		break;
	default:
		break;
	}
}

