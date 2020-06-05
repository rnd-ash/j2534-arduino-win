#include "device.h"
#include "Logger.h"

unsigned long device_table::free_id = 1;
unsigned long device_table::free_chan_id = 1;

device_table::device_table()
{
	this->device_list = std::map<unsigned long, device>();
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
	return handler->send_payload(pMsg, pNumMsgs, Timeout);
}

