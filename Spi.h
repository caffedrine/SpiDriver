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

enum class SPI_MODE
{

};

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
	const int RECV_BUFFER = 512;
	
	Spi(const char *dev, bool AutoRead = false);
	~Spi();
	
	int Write(char *data, int len);
	void Read();
	int IsAvailable();
	
	uint8_t GetMode();
	uint8_t SetMode(uint8_t mode);
	uint8_t SetBits(uint8_t bits);
	uint8_t GetBits();
	uint32_t SetSpeedHz(uint32_t speed);
	uint32_t GetSpeedHz();
	
	
protected:
	virtual void DataReceived(const char *data);
	
private:
	int spifd;
	unsigned baud, bits;
	bool AutoRead = false;
	
	static int SetSocketBlockingEnabled(int fd, int blocking);
};


#endif //_SPI_H
