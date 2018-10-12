cmake_minimum_required(VERSION 3.12)
project(SpiDriver)

set(CMAKE_CXX_STANDARD 11)

add_executable(SpiDriver main.cpp Spi.cpp Spi.h)

#target_link_libraries(SpiDriver pigpio)
target_link_libraries(SpiDriver rt)
target_link_libraries(SpiDriver pthread)