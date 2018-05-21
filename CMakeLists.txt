cmake_minimum_required(VERSION 3.10)
project(ppp_lwip)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

include_directories(serial/include/)
include_directories(serial/include/serial)
include_directories(serial/include/serial/impl)
include_directories(lwip/include/)
include_directories(lwip_nat)
include_directories(pcap/Include)
include_directories(.)

#link_directories(${CMAKE_SOURCE_DIR}/pcap/Lib)
link_directories(${CMAKE_SOURCE_DIR}/pcap/Lib/x64)

add_definitions(-D_WIN32)
#add_definitions(-D__DOXYGEN__)
set(LWIPDIR $PROJECT_SOURCE_DIR/lwip)
include(lwip/Filelists.mk)
#set(SOURCES ${LWIPNOAPPSFILES})
set(SOURCES main.cc
        serial/src/serial.cc
        serial/src/impl/win.cc
        serial/src/impl/list_ports/list_ports_win.cc
        sys_arch.c
        sio.c
        pcapif.c
        pcapif_helper.c
        lwip_main.c
        lwip_nat/ipv4_nat.c)
list(APPEND SOURCES ${LWIPNOAPPSFILES})
list(APPEND SOURCES ${MDNSFILES})
add_executable(ppp_lwip ${SOURCES})

target_link_libraries(${PROJECT_NAME} setupapi)
target_link_libraries(${PROJECT_NAME} wpcap)
target_link_libraries(${PROJECT_NAME} packet)