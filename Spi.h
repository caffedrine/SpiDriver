//
// Created by curiosul on 10/11/18.
//

#ifndef _SPI_H
#define _SPI_H

#include <iostream>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

class Spi
{
	struct Exception : public std::exception
	{
		std::string s;
		explicit Exception(std::string ss): s(ss) {}
		Exception(std::string function, std::string ss) : s( function + "(): " + ss ) {}
		~Exception() throw () {} // Updated
		const char* what() const throw() override { return s.c_str(); }
	};
	
public:
	enum MODE_BIT
	{
		CPHA 		= SPI_CPHA,
		CPOL 		= SPI_CPOL,
		CS_HIGH 	= SPI_CS_HIGH,
		LSB_FIRST 	= SPI_LSB_FIRST,
		THREE_WIRE	= SPI_3WIRE,
		LOOP 		= SPI_LOOP,
		NO_CS 		= SPI_NO_CS,
		READY 		= SPI_READY
	};
	static const int RECV_BUFFER_SIZE = 64;
	
	Spi(const char *dev, bool AutoRead = false);
	~Spi();
	
	int Write(const char *data, int len);
	int Read(const char *data, int max_len);
	int IsAvailable();
	
	uint8_t GetMode();
	uint8_t SetMode(uint8_t mode);
	uint8_t SetBits(uint8_t bits);
	uint8_t GetBits();
	uint32_t SetSpeedHz(uint32_t speed);
	uint32_t GetSpeedHz();
	
	void BackgroundWork();
protected:
	virtual void DataReceived(const char *data, int len);
	
private:
	int spifd;
	unsigned baud, bits;
	bool AutoRead = false;
	int RecvBytes = 0;
	char RecvBuffer[RECV_BUFFER_SIZE];
	
	bool BgThreadCancelToken = false;
	
	static int SetSocketBlockingEnabled(int fd, int blocking);
};


#endif //_SPI_H
