//
// Created by curiosul on 10/11/18.
//

#include <cstdlib>
#include <cstdio>
#include <exception>
#include <cstring>
#include <thread>

#include "Spi.h"

Spi::Spi(const char *dev)
{
	this->spidev_fd = open(dev, O_RDWR);
	if( this->spidev_fd < 0 )
	{
		std::string desc = "open( \"";
		desc += std::string(dev);
		desc += "\" ): ";
		desc += strerror(errno);
		throw Exception(desc);
	}
	
	/* Default settings */
	this->SetBitsPerWord(8);
	this->SetSpeedHz(500000);
	/*Default mode */
//	uint8_t mode = 0b00000000;
//	//mode |= Spi::MODE_BIT::THREE_WIRE;
//	this->SetMode(mode);
}

Spi::~Spi()
{
	this->BgThreadCancelToken = true;
	close(spidev_fd);
	spidev_fd = 0;
}

//	 ____       __ __        __
//	|  _ \     / / \ \      / /
//	| |_) |   / /   \ \ /\ / /
//	|  _ <   / /     \ V  V /
//	|_| \_\ /_/       \_/\_/


int Spi::Read(char *rx_buffer, const int n_words)
{
	uint8_t bits_per_word = this->GetBitsPerWord();
	if( bits_per_word < 0 )
		return bits_per_word;
	
	// Round up to the next biggest number of bytes:
	uint32_t n_bytes = (uint32_t) (((float) (bits_per_word * n_words)) / 8.0 + 0.5);
	if( n_bytes <= 0 )
		return 0;
	if( n_bytes > MAX_TRANSFER_SIZE )
		n_bytes = MAX_TRANSFER_SIZE;
	
	struct spi_ioc_transfer transfer;
	memset((void *) &transfer, 0, sizeof(struct spi_ioc_transfer));
	transfer.tx_buf = 0;
	transfer.rx_buf = (uintptr_t) rx_buffer;
	transfer.len = n_bytes;
	transfer.speed_hz = 0;
	transfer.delay_usecs = 0;
	transfer.bits_per_word = bits_per_word;
	transfer.cs_change = 0;
	if( ioctl(spidev_fd, SPI_IOC_MESSAGE(1), &transfer) < 0 )
	{
		throw Exception("Read", strerror(errno));
	}
	return (n_bytes << 3) / bits_per_word;
}

int Spi::Write(const char *tx_data, const int n_words)
{
	uint8_t bits_per_word = GetBitsPerWord();
	if( bits_per_word < 0 )
		throw Exception("Write", "Invalid bits per word value!");
	
	/* Calculate number of bytes to transfer */
	uint32_t n_bytes = (uint32_t) (((float) (bits_per_word * n_words)) / 8.0 + 0.5);
	if( n_bytes <= 0 )
		return 0;
	else if( n_bytes > MAX_TRANSFER_SIZE )
		n_bytes = MAX_TRANSFER_SIZE;
	
	/* Structure to transfer over the wires */
	struct spi_ioc_transfer transfer;
	memset((void *) &transfer, 0, sizeof(struct spi_ioc_transfer));
	transfer.tx_buf = (uintptr_t) tx_data;
	transfer.rx_buf = 0;
	transfer.len = n_bytes;
	transfer.speed_hz = 0;
	transfer.delay_usecs = 0;
	transfer.bits_per_word = bits_per_word;
	transfer.cs_change = 0;
	
	int writeStatus = ioctl(spidev_fd, SPI_IOC_MESSAGE(1), &transfer);
	if( writeStatus < 0 )
	{
		throw Exception("Write", strerror(errno));
	}
	return 1;
}

int Spi::Transaction(char *tx_data, int tx_n_words, char *rx_data, int rx_n_words)
{
	/** Send then receive */
	uint8_t bits_per_word;
	uint32_t n_tx_bytes, n_rx_bytes;
	struct spi_ioc_transfer transfers[2];
	int n_transfers;
	bits_per_word = GetBitsPerWord();
	if( bits_per_word < 0 )
		return bits_per_word;
	// Round up to the next biggest number of bytes:
	n_tx_bytes = (uint32_t) (((float) (bits_per_word * tx_n_words)) / 8.0 + 0.5);
	n_rx_bytes = (uint32_t) (((float) (bits_per_word * rx_n_words)) / 8.0 + 0.5);
	if( !n_rx_bytes && !n_tx_bytes )
		return 0;
	if( n_rx_bytes > MAX_TRANSFER_SIZE )
		n_rx_bytes = MAX_TRANSFER_SIZE;
	if( n_tx_bytes > MAX_TRANSFER_SIZE )
		n_tx_bytes = MAX_TRANSFER_SIZE;
	
	n_transfers = 0;
	if( n_tx_bytes )
	{
		memset((void *) &transfers[n_transfers], 0,
			   sizeof(struct spi_ioc_transfer));
		transfers[n_transfers].tx_buf = (uintptr_t) tx_data;
		transfers[n_transfers].rx_buf = 0;
		transfers[n_transfers].len = n_tx_bytes;
		transfers[n_transfers].speed_hz = 0;
		transfers[n_transfers].delay_usecs = 0;
		transfers[n_transfers].bits_per_word = bits_per_word;
		transfers[n_transfers].cs_change = 0;
		
		++n_transfers;
	}
	
	if( n_rx_bytes )
	{
		memset((void *) &transfers[n_transfers], 0, sizeof(struct spi_ioc_transfer));
		transfers[n_transfers].tx_buf = 0;
		transfers[n_transfers].rx_buf = (uintptr_t) rx_data;
		transfers[n_transfers].len = n_rx_bytes + 1;
		transfers[n_transfers].speed_hz = 0;
		transfers[n_transfers].delay_usecs = 0;
		transfers[n_transfers].bits_per_word = bits_per_word;
		transfers[n_transfers].cs_change = 0;
		
		++n_transfers;
	}
	
	if( ioctl(spidev_fd, SPI_IOC_MESSAGE(n_transfers), transfers) < 0 )
	{
		throw Exception("Transaction", strerror(errno));
	}
	return (n_rx_bytes << 3) / bits_per_word;
}

int Spi::Transfer(char *tx_data, char *rx_data, char n_words)
{
	/** Send and receive simultaneously */
	uint8_t bits_per_word = this->GetBitsPerWord();
	if (bits_per_word < 0)
		return bits_per_word;
	uint32_t n_bytes = (uint32_t) (((float) (bits_per_word * n_words)) / 8.0 + 0.5);
	if (!n_bytes)
		return 0;
	if (n_bytes > MAX_TRANSFER_SIZE)
		n_bytes = MAX_TRANSFER_SIZE;
	
	struct spi_ioc_transfer transfer;
	memset((void *) &transfer, 0, sizeof(struct spi_ioc_transfer));
	transfer.tx_buf = (uintptr_t) tx_data;
	transfer.rx_buf = (uintptr_t) rx_data;
	transfer.len = n_bytes;
	transfer.speed_hz = 0;
	transfer.delay_usecs = 0;
	transfer.bits_per_word = bits_per_word;
	transfer.cs_change = 0;
	if (ioctl(spidev_fd, SPI_IOC_MESSAGE(1), &transfer) < 0)
	{
		throw Exception("Transfer",  strerror(errno));
	}
	return (n_bytes<<3) / bits_per_word;
}

int Spi::IsAvailable()
{
	/* Check the interruption pin here */
	return 0;
}

//	   _____ ____  ____     _____ _______________________   _____________
//	  / ___// __ \/  _/    / ___// ____/_  __/_  __/  _/ | / / ____/ ___/
//	  \__ \/ /_/ // /      \__ \/ __/   / /   / /  / //  |/ / / __ \__ \
//	 ___/ / ____// /      ___/ / /___  / /   / / _/ // /|  / /_/ /___/ /
//  /____/_/   /___/     /____/_____/ /_/   /_/ /___/_/ |_/\____//____/
//
uint8_t Spi::SetBitsPerWord(uint8_t bits)
{
	if( ioctl(this->spidev_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1 )
	{
		throw Exception("SetBitsPerWord", strerror(errno));
	}
	
	uint8_t BitsSet;
	if( ioctl(this->spidev_fd, SPI_IOC_RD_BITS_PER_WORD, &BitsSet) == -1 )
	{
		throw Exception("SetBitsPerWord", strerror(errno));
	}
	return BitsSet;
}

uint8_t Spi::GetBitsPerWord()
{
	uint8_t BitsSet;
	if( ioctl(this->spidev_fd, SPI_IOC_RD_BITS_PER_WORD, &BitsSet) == -1 )
	{
		throw Exception("GetBitsPerWord", strerror(errno));
	}
	return BitsSet;
}

uint32_t Spi::SetSpeedHz(uint32_t speed)
{
	if( ioctl(spidev_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1 )
		throw Exception("SetSpeedHz", strerror(errno));
	
	uint32_t SpeedSet;
	if( ioctl(spidev_fd, SPI_IOC_RD_MAX_SPEED_HZ, &SpeedSet) == -1 )
		throw Exception("SetSpeedHz", strerror(errno));
	return SpeedSet;
}

uint32_t Spi::GetSpeedHz()
{
	uint32_t SpeedSet;
	if( ioctl(spidev_fd, SPI_IOC_RD_MAX_SPEED_HZ, &SpeedSet) == -1 )
		throw Exception("GetSpeedHz", strerror(errno));
	return SpeedSet;
}

//	 ____  ____ ___    __  __  ___  ____  _____ ____
//	/ ___||  _ \_ _|  |  \/  |/ _ \|  _ \| ____/ ___|
//	\___ \| |_) | |   | |\/| | | | | | | |  _| \___ \
//	 ___) |  __/| |   | |  | | |_| | |_| | |___ ___) |
//	|____/|_|  |___|  |_|  |_|\___/|____/|_____|____/


uint8_t Spi::GetMode()
{
	uint8_t ModeSet;
	if( ioctl(spidev_fd, SPI_IOC_RD_MODE, &ModeSet) == -1 )
		throw Exception("GetMode", strerror(errno));
	return ModeSet;
}

uint8_t Spi::SetMode(uint8_t mode)
{
	/* Possible modes:
		 * mode |= SPI_LOOP;
		 * mode |= SPI_CPHA;
		 * mode |= SPI_CPOL;
		 * mode |= SPI_LSB_FIRST;
		 * mode |= SPI_CS_HIGH;
		 * mode |= SPI_3WIRE;
		 * mode |= SPI_NO_CS;
		 * mode |= SPI_READY;
	 */
	if( ioctl(spidev_fd, SPI_IOC_WR_MODE, &mode) == -1 )
		throw Exception("SetMode", strerror(errno));
	
	uint8_t ModeSet;
	if( ioctl(spidev_fd, SPI_IOC_RD_MODE, &ModeSet) == -1 )
		throw Exception("SetMode", strerror(errno));
	return ModeSet;
}

int Spi::SetClockMode(uint8_t clock_mode)
{
	uint8_t mode;
	mode = this->GetMode();
	if( mode < 0 )
		return mode;
	mode &= ~0x3;
	mode |= clock_mode & 0x3;
	return this->SetMode(mode);
}

int Spi::GetClockMode()
{
	uint8_t clock_mode;
	clock_mode = this->GetMode();
	if( clock_mode < 0 )
		return clock_mode;
	return clock_mode & 0x3;
}

int Spi::SetCSActiveLow()
{
	int mode = this->GetMode();
	if( mode < 0 )
		return -1;
	return this->SetMode(mode & ~SPI_CS_HIGH);
}

int Spi::SetCSActiveHigh()
{
	int mode = this->GetMode();
	if( mode < 0 )
		return -1;
	return this->SetMode(mode | SPI_CS_HIGH);
}

int Spi::EnableCS()
{
	int mode = this->GetMode();
	if( mode < 0 )
		return -1;
	return this->SetMode(mode & ~SPI_NO_CS);
}

int Spi::DisableCS()
{
	int mode = this->GetMode();
	if( mode < 0 )
		return -1;
	return this->SetMode(mode | SPI_NO_CS);
}

int Spi::EnableLoopback()
{
	int mode = this->GetMode();
	if( mode < 0 )
		return -1;
	return this->SetMode(mode | SPI_LOOP);
}

int Spi::DisableLoopback()
{
	int mode = this->GetMode();
	if( mode < 0 )
		return -1;
	return this->SetMode(mode & ~SPI_LOOP);
}

int Spi::Enable3Wire()
{
	int mode = this->GetMode();
	if( mode < 0 )
		return -1;
	return this->SetMode(mode | SPI_3WIRE);
}

int Spi::Disable3Wire()
{
	int mode = this->GetMode();
	if( mode < 0 )
		return -1;
	return this->SetMode(mode & ~SPI_3WIRE);
}


void Spi::DataReceived(const char *data, const int length)
{
	std::cout << "RECV (" << length << " bytes): " << data << std::endl;
//	for(int i = 0; i < length; i++)
//		std::cout << "0x" << (int)data[i] << " ";
//	std::cout << "\n";
}

void Spi::BackgroundWork()
{
	while( true )
	{
		if( this->BgThreadCancelToken == true )
			break;
		
		if( this->IsAvailable() > 0 )
		{
			this->RecvBytes = (int) read(this->spidev_fd, this->RecvBuffer, this->MAX_TRANSFER_SIZE);
			
			if( this->RecvBytes < 0 )
				throw Exception("read", strerror(errno));
			else if( this->RecvBytes > 0 )
			{
				//sDataReceived(this->RecvBuffer, this->RecvBytes);
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

