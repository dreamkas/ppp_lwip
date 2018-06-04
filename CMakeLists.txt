cmake_minimum_required(VERSION 3.10)
project(ppp_lwip)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

# собрать приложение в виде службы
# Сейчас по этому флагу только подтягиваются исходники примера службы. Код в службу не интегрирован.
#set(build_as_service on)
add_definitions(-D_WIN32_WINNT=0x600)
if (${build_as_service})
    set(CMAKE_EXE_LINKER_FLAGS "-municode -static")
    set(CMAKE_CXX_FLAGS "-fpermissive -Wwrite-strings")
    add_definitions(-DUNICODE -D_UNICODE -DBUILD_AS_SERVICE)
    include_directories(windows_service)
else () #для установки флагов компилятора без службы
    #set(CMAKE_CXX_FLAGS "-fpermissive")
    set(CMAKE_EXE_LINKER_FLAGS "-static")
endif ()

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
        lwip_nat/ipv4_nat.c
        interfaces.cpp)
list(APPEND SOURCES ${LWIPNOAPPSFILES})
list(APPEND SOURCES ${MDNSFILES})

if (${build_as_service})
    list(APPEND SOURCES
            windows_service/CppWindowsService.cpp
            windows_service/SampleService.cpp
            windows_service/ServiceBase.cpp
            windows_service/ServiceInstaller.cpp)
endif ()

add_executable(ppp_lwip ${SOURCES})

target_link_libraries(${PROJECT_NAME} setupapi)
target_link_libraries(${PROJECT_NAME} wpcap)
target_link_libraries(${PROJECT_NAME} packet)
target_link_libraries(${PROJECT_NAME} iphlpapi)
target_link_libraries(${PROJECT_NAME} Ws2_32)