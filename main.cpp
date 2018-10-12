#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>

#include "Spi.h"

int main()
{
	std::cout << "---started---\n";
	system("modprobe spi-bcm2835");
	
	Spi spi0("/dev/spidev0.1",  false);
	spi0.SetSpeedHz(1000000);
	spi0.SetBits(8);
	
	uint8_t mode = 0b00000000;
	//mode |= Spi::MODE_BIT::NO_CS;
	spi0.SetMode(mode);
	
//	while(1)
//	{
//		if(spi0.IsAvailable() > 0)
//		{
//			char buff[512];
//			int readBytes = spi0.Read(buff, 512);
//			std::cout << "RECV (" << readBytes << " bytes): " << buff << std::endl;
//		}
//		std::this_thread::sleep_for( std::chrono::milliseconds(1) );
//	}
	
	spi0.BackgroundWork();
	
	while(true)
	{
		char msg[] = "Alex!\r\n";
		spi0.Write(msg, strlen(msg));
		
		std::this_thread::sleep_for( std::chrono::milliseconds(1) );
	}
	return 0;
}