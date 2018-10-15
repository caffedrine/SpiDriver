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
	enum class BitOrder
	{
		SPI_MSBFIRST,
		SPI_LSBFIRST
	};
	static const int MAX_TRANSFER_SIZE = 8192;
	
	Spi(const char *dev);
	~Spi();
	
	int Write(const char *data, int n_words);
	int Read(char *rx_buffer, int n_words);
	/** Send then receive */
	int Transaction(char *tx_data, int tx_n_words, char *rx_data, int rx_n_words);
	/** Send and receive simultaneously */
	int Transfer(char *tx_data, char *rx_data, char n_words);
	
	int IsAvailable();
	
	/* Spi settings */
	uint8_t SetBitsPerWord(uint8_t s_bits);
	uint8_t GetBitsPerWord();
	uint32_t SetSpeedHz(uint32_t speed);
	uint32_t GetSpeedHz();
	
	/* Modes settings */
	uint8_t GetMode();
	uint8_t SetMode(uint8_t mode);
	int SetClockMode(uint8_t mode);
	int GetClockMode();
	int SetCSActiveLow();
	int SetCSActiveHigh();
	int EnableCS();
	int DisableCS();
	int EnableLoopback();
	int DisableLoopback();
	int Enable3Wire();
	int Disable3Wire();
	
	void BackgroundWork();
protected:
	virtual void DataReceived(const char *data, int len);
	
private:
	uint8_t mode;
	int spidev_fd;
	bool AutoRead = false;
	int RecvBytes = 0;
	char RecvBuffer[MAX_TRANSFER_SIZE];
	
	bool BgThreadCancelToken = false;
	int Words2Bytes(int words);
};


#endif //_SPI_H
