//
// Created by Gegel85 on 13/07/2019.
//

#include <inaddr.h>
#include <cassert>
#include "BgbHandler.hpp"
#include "Exception.hpp"

BGBHandler::BGBHandler(
	const std::function<unsigned char(EmulatorHandle &handler, unsigned char byte)> &masterHandler,
	const std::function<unsigned char(EmulatorHandle &handler, unsigned char byte)> &slaveHandler,
	const std::string &ip,
	unsigned short port,
	bool log
) :
	EmulatorHandle(masterHandler, slaveHandler, log),
	_mainHandler([this] { while (this->_handleLoop()); })
{
	this->log("Connecting to " + ip + ":" + std::to_string(port));
	this->_socket.connect(ip, port);
	this->_disconnected = false;
	this->log("Performing handshake");
	this->_sendPacket({VERSION_CHECK, 1, 4, 0, 0});
	this->_mainThread = std::thread(this->_mainHandler);
}

BGBHandler::~BGBHandler()
{
	this->disconnect();
}

void BGBHandler::disconnect()
{
	this->log("Disconnecting...");
	this->_disconnected = true;
	if (this->_mainThread.joinable())
		this->_mainThread.join();
}

bool BGBHandler::isConnected()
{
	return !this->_disconnected;
}

void BGBHandler::log(const std::string &string, std::ostream &stream)
{
	while (this->_logging);
	this->_logging = true;
	if (this->_log)
		stream << "[BGBHandler]: " << string << std::endl;
	this->_logging = false;
}

void BGBHandler::sendByte(unsigned char byte)
{
	this->log("Sending " + std::to_string(byte));
	this->_sendPacket({SYNC1_SIGNAL, byte, 0x80, 0, this->_ticks * 1024});
}

void BGBHandler::reply(unsigned char byte)
{
	this->log("Replying " + std::to_string(byte));
	this->_sendPacket({SYNC2_SIGNAL, byte, 0x80, 0, 0});
}

void BGBHandler::_sendPacket(const BGBHandler::BGBPacket &packet)
{
	char buffer[8];

	buffer[0] = packet.b1;
	buffer[1] = packet.b2;
	buffer[2] = packet.b3;
	buffer[3] = packet.b4;
	buffer[4] = (static_cast<unsigned char>(packet.i1) >> 0LU);
	buffer[5] = (static_cast<unsigned char>(packet.i1) >> 8LU);
	buffer[6] = (static_cast<unsigned char>(packet.i1) >> 16LU);
	buffer[7] = (static_cast<unsigned char>(packet.i1) >> 24LU);
	this->_socket.send({
		buffer,
		sizeof(buffer)
	});
}

BGBHandler::BGBPacket BGBHandler::_getNextPacket()
{
	std::string serverMessage;

	serverMessage = this->_socket.read(PACKET_SIZE);

	assert(serverMessage.length() == 8);

	return {
		static_cast<unsigned char>(serverMessage[0]),
		static_cast<unsigned char>(serverMessage[1]),
		static_cast<unsigned char>(serverMessage[2]),
		static_cast<unsigned char>(serverMessage[3]),
		static_cast<unsigned int>(
			(static_cast<unsigned char>(serverMessage[4]) << 0LU) +
			(static_cast<unsigned char>(serverMessage[5]) << 8LU) +
			(static_cast<unsigned char>(serverMessage[6]) << 16LU)+
			(static_cast<unsigned char>(serverMessage[7]) << 24LU)
		)
	};
}


void BGBHandler::_sync()
{
	this->_sendPacket({SYNC3_SIGNAL, 0, 0, 0, ++this->_ticks * 1024});
}

bool BGBHandler::_handleLoop()
{
	if (this->_disconnected || !this->_socket.isOpen())
		return false;

	BGBPacket packet;

	try {
		packet = this->_getNextPacket();
	} catch (EOFException &) {
		return false;
	}

	switch (packet.b1) {
	case VERSION_CHECK:
		if (packet.b2 != 1 || packet.b3 != 4 || packet.b4 != 0 || packet.i1 != 0) {
			this->log("Server version is invalid");
			throw InvalidVersionException("Server version is not compatible");
		}
		this->_sendPacket({STATUS, STATUSFLAG_RUNNING & STATUSFLAG_PAUSED, 0, 0, 0});
		this->log("Server version is OK");
		return true;

	case SYNC1_SIGNAL:
		this->log("Received one byte (" + std::to_string(packet.b2) + ") as master");
		if (this->_masterHandler)
			packet.b2 = this->_masterHandler(*this, packet.b2);
		this->_sendPacket(packet);
		return true;

	case SYNC2_SIGNAL:
		this->log("Received one byte (" + std::to_string(packet.b2) + ") as slave");
		if (this->_slaveHandler)
			packet.b2 = this->_slaveHandler(*this, packet.b2);
		this->_sendPacket(packet);
		this->_sync();
		return true;

	case SYNC3_SIGNAL:
		this->_sync();
		return true;

	case STATUS:
		this->_sendPacket({STATUS, STATUSFLAG_RUNNING, 0, 0, 0});
		this->_sync();
		return true;

	case JOYPAD_CHANGE:
	case WANT_DISCONNECT:
		return true;
	default:
		this->log("Unknown command sent by server (Opcode: " + std::to_string(packet.b1) + ")");
	}
	return true;
}