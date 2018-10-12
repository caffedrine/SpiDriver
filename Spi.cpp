//
// Created by curiosul on 10/11/18.
//

#include <cstdlib>
#include <cstdio>
#include <exception>
#include <cstring>
#include "Spi.h"

Spi::Spi(const char *dev, bool auto_read)
{
	this->spifd = open(dev, O_RDWR);
	if( this->spifd < 0 )
	{
		std::string desc = "open( \"";
		desc += std::string(dev);
		desc += "\" ): ";
		desc += strerror(errno);
		throw Exception(desc);
	}
	this->AutoRead = auto_read;
	
	/* Default settings */
	this->SetBits(8);
	this->SetSpeedHz(500000);
}

Spi::~Spi()
{
	close(spifd);
	spifd = 0;
}

int Spi::Write(char *data, int len)
{
	struct spi_ioc_transfer xfer;
	xfer.tx_buf = (unsigned long) data;
	xfer.len = len;
	xfer.rx_buf = 0;
	xfer.delay_usecs = 0;
	xfer.speed_hz = 0;
	xfer.bits_per_word = 0;
	
	int writeStatus = ioctl(spifd, SPI_IOC_MESSAGE(1), &xfer);
	if( writeStatus < 0 )
	{
		return -1;
	}
	return 1;
}

void Spi::Read()
{
	char buffer[this->RECV_BUFFER];
	
	if( read(spifd, buffer, this->RECV_BUFFER) < 0 )
	{
		std::string desc = "read(): ";
		desc += strerror(errno);
		throw Exception(desc);
	}
	DataReceived(buffer);
}

void Spi::DataReceived(const char *data)
{
	std::cout << "RECV: " << data << std::endl;
}

int Spi::SetSocketBlockingEnabled(int fd, int blocking)
{
	if( fd < 0 ) return -1;
	
	int flags = fcntl(fd, F_GETFL, 0);
	if( flags == -1 ) return -1;
	flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
	return (fcntl(fd, F_SETFL, flags) == 0) ? 0 : -1;
}

int Spi::IsAvailable()
{
	fd_set sockset;
	FD_ZERO(&sockset);
	FD_SET(this->spifd, &sockset);
	int result = select(this->spifd + 1, &sockset, NULL, NULL, NULL);
	if( result == 1 )
	{
		// The socket has data. For good measure, it's not a bad idea to test further
		if( FD_ISSET(this->spifd, &sockset) )
		{
			///TODO: Investigate
//			int availableBytes = 0;
//			ioctl(sockfd, FIONREAD, &availableBytes);
//			if( availableBytes < sizeof(struct can_frame) )
//				return 0;
//			return (availableBytes / sizeof(struct can_frame));
			return 1;
		}
	}
	return 0;
}

uint8_t Spi::SetBits(uint8_t bits)
{
	if( ioctl(this->spifd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1 )
	{
		throw Exception("SetBits", strerror(errno));
	}
	
	uint8_t BitsSet;
	if( ioctl(this->spifd, SPI_IOC_RD_BITS_PER_WORD, &BitsSet) == -1 )
	{
		throw Exception("SetBits", strerror(errno));
	}
	return BitsSet;
}

uint8_t Spi::GetBits()
{
	uint8_t BitsSet;
	if( ioctl(this->spifd, SPI_IOC_RD_BITS_PER_WORD, &BitsSet) == -1 )
	{
		throw Exception("GetBits", strerror(errno));
	}
	return BitsSet;
}

uint32_t Spi::SetSpeedHz(uint32_t speed)
{
	if( ioctl(spifd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1 )
		throw Exception("SetSpeedHz", strerror(errno));
	
	uint32_t SpeedSet;
	if( ioctl(spifd, SPI_IOC_RD_MAX_SPEED_HZ, &SpeedSet) == -1 )
		throw Exception("SetSpeedHz", strerror(errno));
	return SpeedSet;
}

uint32_t Spi::GetSpeedHz()
{
	uint32_t SpeedSet;
	if( ioctl(spifd, SPI_IOC_RD_MAX_SPEED_HZ, &SpeedSet) == -1 )
		throw Exception("GetSpeedHz", strerror(errno));
	return SpeedSet;
}

uint8_t Spi::GetMode()
{
	uint8_t ModeSet;
	if( ioctl(spifd, SPI_IOC_RD_MODE, &ModeSet) == -1 )
		throw Exception("GetMode", strerror(errno));
	return ModeSet;
}

uint8_t Spi::SetMode(uint8_t mode)
{
	if( ioctl(spifd, SPI_IOC_WR_MODE, &mode) == -1 )
		throw Exception("SetMode", strerror(errno));
	
	uint8_t ModeSet;
	if( ioctl(spifd, SPI_IOC_RD_MODE, &ModeSet) == -1 )
		throw Exception("SetMode", strerror(errno));
	return ModeSet;
}

