//
// Created by Gegel85 on 13/07/2019.
//

#ifndef POKEAI_EMULATORHANDLE_HPP
#define POKEAI_EMULATORHANDLE_HPP


#include <sstream>
#include <iomanip>
#include <iostream>
#include <functional>

#define charToHex(val)\
([](unsigned char i){\
	std::stringstream stream;\
\
	stream << "0x" << std::setfill('0') << std::setw(2) << std::hex << std::uppercase << static_cast<int>(i);\
	return stream.str();\
}(val))

typedef std::function<unsigned char(class EmulatorHandle &handler, unsigned char byte)> ByteHandle;

class EmulatorHandle {
protected:
	bool _log;
	bool _disconnected = true;
	ByteHandle _slaveHandler;
	ByteHandle _masterHandler;

		_log(log),
		_slaveHandler(slaveHandler),
		_masterHandler(masterHandler)
	{};

public:
	virtual void log(const std::string &string, std::ostream &stream = std::cout) = 0;
	virtual void sendByte(unsigned char byte) = 0;
	virtual void reply(unsigned char byte) = 0;
	virtual void disconnect() = 0;

	bool isConnected()
	{
		return !this->_disconnected;
	}
};


#endif //POKEAI_EMULATORHANDLE_HPP
