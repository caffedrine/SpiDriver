#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>

#include "Spi.h"

int main()
{
	std::cout << "---started---\n";
	system("modprobe spi-bcm2835");
	Spi spi0("/dev/spidev0.1");
	
	spi0.Read();
	
	while(true)
	{
		char msg[] = "Alex!\r\n";
		spi0.Write(msg, strlen(msg));
		
		std::this_thread::sleep_for( std::chrono::milliseconds(1) );
	}
	return 0;
}