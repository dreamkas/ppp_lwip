//#include <w32api/vss.h>
#include <iostream>

// OS Specific sleep
#ifdef _WIN32

#include <windows.h>
#include <lwip/ip6.h>
#include <cstdio>
#define HAVE_REMOTE
#include <pcap.h>

#else
#include <unistd.h>
#endif

#include "serial/serial.h"
#include "lwip/include/lwip/ip4_addr.h"
#include "interfaces.h"

#define SERIAL_BAUD (115200)

static const uint32_t SERIAL_BAUDRATE = 115200;
static const uint32_t SERIAL_TIMEOUT = 50; /* ms */
static const char *const MAGIC_REQUEST = "CLIENT";
static const char *const MAGIC_ANSWER = "CLIENTSERVER";
static const char *portPrefix = "COM";
volatile bool pppConnected;
extern u32_t sio_idx;
extern ip4_addr_t ourAddr, hisAddr;
extern int NEW_PACKET_LIB_ADAPTER_NR;
extern u_long ADAPTER_ADDR;
extern u_long ADAPTER_MASK;
extern u_long ADAPTER_GW;
extern u_long ADAPTER_DNS;


using std::string;
using std::exception;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;

static bool validateIpV4(const char *ip);
static bool setSettings(int argc, char **argv, string &port);
static bool waitSerialHandshake(string port);

int getPcapNum(interfaces::param_t &param)
{
    if (interfaces::get(param))
    {
        pcap_if_t *alldevs, *d;
        char errbuf[PCAP_ERRBUF_SIZE + 1];
        if (pcap_findalldevs_ex((char *)"on local host", NULL, &alldevs, errbuf) == -1)
        {
            fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
            return -1;
        }
        int i = 0;
        for (d = alldevs; d; d = d->next)    //Print the devices
        {
            std::string name = d->name;
            if (name.find(param.name) != std::string::npos)
            {
                return i;
            }
            i++;
        }
    }
    else
    {
        interfaces::createLoopback();
        return -1;
    }
    return -1;
}

bool waitSerialHandshake(string port)
{
    try
    {
        serial::Serial serialPort(port, SERIAL_BAUDRATE, serial::Timeout::simpleTimeout(SERIAL_TIMEOUT));

        if (!serialPort.isOpen())
        {
            serialPort.open();
        }

        string readBuf;
        uint8_t ch = 0;

        while (true) /* TODO: Добавить таймаут на поверку одного порта. */
        {
            serialPort.read(&ch, 1);

            readBuf.push_back(ch);

            if (readBuf.find(MAGIC_REQUEST, 0) != string::npos)
            {
                /* Requested string was found. */
//                pppConnected = true;

                cout << MAGIC_REQUEST << " request was received. Sending " << MAGIC_ANSWER << " answer...\n";

                serialPort.write(MAGIC_ANSWER); /* TODO: Check for return code. */
                serialPort.close();

                break;
            }
        }
    }
    catch (exception &e)
    {
        cout << "Exception has happened...sorry. " << e.what() << endl;
        return false;

    }
    return true;
}

static bool validateIpV4(const char *ip)
{
    unsigned int b1, b2, b3, b4;
    unsigned char c;

    if (sscanf(ip, "%3u.%3u.%3u.%3u%c", &b1, &b2, &b3, &b4, &c) != 4)
    {
        return false;
    }

    if (b1 > 255 || b2 > 255 || b3 > 255 || b4 > 255)
    {
        return false;
    }

    if (strspn(ip, "0123456789.") < strlen(ip))
    {
        return false;
    }
    return true;
}

static bool setSettings(int argc, char **argv, string &port)
{
    if (argc < 4)
    {
        return false;
    }

    if (strlen(argv[1]) < strlen(portPrefix) + 1)
    {
        return false;
    }

    if (strncmp(argv[1], portPrefix, strlen(portPrefix)) != 0)
    {
        return false;
    }

    if (!validateIpV4(argv[2]) || !validateIpV4(argv[3]))
    {
        return false;
    }

    // два одинаковых адреса
    if (!strcmp(argv[2], argv[3]))
    {
        return false;
    }

    unsigned int b1, b2, b3, b4;
    port = argv[1];
    sscanf(argv[2], "%3u.%3u.%3u.%3u", &b1, &b2, &b3, &b4);
    IP4_ADDR(&ourAddr, b1, b2, b3, b4);
    sscanf(argv[3], "%3u.%3u.%3u.%3u", &b1, &b2, &b3, &b4);
    IP4_ADDR(&hisAddr, b1, b2, b3, b4);

    try
    {
        string portNum = port.substr(strlen(portPrefix), string::npos);
        sio_idx = static_cast<uint32_t>(stoi(portNum));
    }
    catch (...)
    {
        return false;
    }

    return true;
}

extern "C" void lwipLoop();

void signalHandler(int signum)
{
    cout << "Interrupt signal (" << signum << ") received.\n";
    cout << "program dead.\n";

    exit(signum);
}

int main(int argc, char **argv)
{
    string port;

    // register signal SIGINT and signal handler
    signal(SIGINT, signalHandler);

    if (!setSettings(argc, argv, port))
    {
        printf("Settings not valid!\nExample: COM1 192.168.1.2 192.168.1.3\n");
        return -1;
    }



    interfaces::param_t param;
    int num = getPcapNum(param);

    if (num < 0)
    {
        exit(1);
    }
    else
    {
        NEW_PACKET_LIB_ADAPTER_NR = num;
        ADAPTER_ADDR = param.addr;
        ADAPTER_MASK = param.mask;
        ADAPTER_GW = param.gateway;
        ADAPTER_DNS = param.dns;
    }

    while (true)
    {
        if (!waitSerialHandshake(port))
        {
            Sleep(500);
            continue;
        }

        printf("Using serial port %d for PPP\n", sio_idx);

        /* no stdio-buffering, please! */
        setvbuf(stdout, NULL, _IONBF, 0);

        lwipLoop();
    }

    puts("program dead.\n");

    return 0;
}

