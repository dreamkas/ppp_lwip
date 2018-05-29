//#include <w32api/vss.h>
#include <iostream>

// OS Specific sleep
#ifdef _WIN32

#include <windows.h>
#include <lwip/ip6.h>
#include <cstdio>

#else
#include <unistd.h>
#endif

#include "serial/serial.h"
#include "lwip/include/lwip/ip4_addr.h"

#define SERIAL_BAUD (115200)

bool isPPPConnected;
extern u8_t sio_idx;
extern ip4_addr_t ourAddr, hisAddr;

using std::string;
using std::exception;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;


bool initSerial()
{
    return false;
}

void enumerate_ports()
{
    auto devices_found = serial::list_ports();

    auto iter = devices_found.begin();

    while (iter != devices_found.end())
    {
        serial::PortInfo device = *iter++;

        printf("(%s, %s, %s)\n", device.port.c_str(), device.description.c_str(), device.hardware_id.c_str());
    }
}

void print_usage()
{
    cerr << "Usage: test_serial {-e|<serial port address>} ";
    cerr << "<baudrate> [test string]" << endl;
}

int run(const std::string port, unsigned long baud)
{
    if (port == "-e")
    {
        enumerate_ports();
        return 0;
    }

    // port, baudrate, timeout in milliseconds
    serial::Serial my_serial(port, baud, serial::Timeout::simpleTimeout(1000));

    cout << "Is the serial port open?";
    if (my_serial.isOpen())
    {
        cout << " Yes." << endl;
    }
    else
    {
        cout << " No." << endl;
    }

    // Get the Test string
    int count = 0;
    uint8_t test_string[] = {0xAB, 0xCD, 0xFF};
//    if (argc == 4)
//    {
//        test_string = argv[3];
//    }
//    else
//    {
//        test_string = {0xAB, 0xCD, 0xFF};
//    }
    uint8_t read_buf[128] = {0};
    // Test the timeout, there should be 1 second between prints
    cout << "Timeout == 1000ms, asking for 1 more byte than written." << endl;
    while (count < 10)
    {
        size_t bytes_wrote = my_serial.write(test_string, sizeof(test_string));

        my_serial.read(read_buf, sizeof(read_buf));

//        cout << "Iteration: " << count << ", Bytes written: ";
//        cout << bytes_wrote << ", Bytes read: ";
//        cout << "Bytes read: " << result.length() << ", String read: " << result << endl;

        count += 1;
    }

    // Test the timeout at 250ms
    my_serial.setTimeout(serial::Timeout::max(), 250, 0, 250, 0);

    return 0;
}

int waitPPPConnect(const std::string port, unsigned long baud)
{
    if (port == "-e")
    {
        enumerate_ports();
        return 0;
    }

    // port, baudrate, timeout in milliseconds
    serial::Serial my_serial(port, baud, serial::Timeout::simpleTimeout(50));

    cout << "Is the serial port open?";
    if (my_serial.isOpen())
    {
        cout << " Yes." << endl;
    }
    else
    {
        cout << " No." << endl;
    }

    // Get the Test string
    int count = 0;
    string readBuf;
    uint8_t ch = 0;
    string magicRequest = "CLIENT";
    string magicAnswer = "CLIENTSERVER";

    while (true)
    {
        if (my_serial.read(&ch, 1))
        {
            readBuf.push_back(ch);

            if (readBuf.length() > magicRequest.length())
            {
                if (readBuf.find(magicRequest, 0) != string::npos)
                {
                    /* Requested string was found. */
                    isPPPConnected = true;

                    cout << magicRequest << " request was received. Sending " << magicAnswer << " answer...\n";

                    my_serial.write(magicAnswer); /* TODO: Check for return code. */
                    my_serial.close();

                    break;
                }
                else
                {
                    readBuf.erase(0, 1);
                }
            }
        }
    }


//    // Test the timeout, there should be 1 second between prints
//    cout << "Timeout == 1000ms, asking for 1 more byte than written." << endl;
//    while (count < 10)
//    {
////        size_t bytes_wrote = my_serial.write(test_string);
//
//        string result = my_serial.read(128);
//
////        cout << "Iteration: " << count << ", Bytes written: ";
////        cout << bytes_wrote << ", Bytes read: ";
//        cout << "Bytes read: " << result.length() << ", String read: " << result << endl;
//
//        count += 1;
//    }
//
//    // Test the timeout at 250ms
//    my_serial.setTimeout(serial::Timeout::max(), 250, 0, 250, 0);
}

void initPPPServer()
{

}

//int main(int argc, char **argv)
//{
//
//    waitPPPConnect(argc, argv);
//
//    initPPPServer();
//
//
////    /* TODO: Initialise LwIP. */
////
////    if (!initSerial())
////    {
////        cout << "Cannot initialize serial port!" << endl;
////        return -1;
////    }
////
////    while (true)
////    {
////        /* TODO: LwIP cycle. */
////    }
////    try
////    {
////        return waitPPPConnect(argc, argv);
////    }
////    catch (exception &e)
////    {
////        cerr << "Unhandled Exception: " << e.what() << endl;
////    }
//
//    return 0;
//}

bool validateIpV4(const char* ip)
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

bool validateSettings(int argc, char **argv)
{
    if (argc < 4)
    {
        return false;
    }

    if (strcmp(argv[1], "-e") && strncmp(argv[1], "COM", 3))
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

    return true;
}

extern "C" void main_loop();


int main(int argc, char **argv)
{
    string port;

    if (validateSettings(argc, argv))
    {
        unsigned int b1, b2, b3, b4;
        port = argv[1];
        sscanf(argv[2], "%3u.%3u.%3u.%3u", &b1, &b2, &b3, &b4);
        IP4_ADDR(&ourAddr, b1, b2, b3, b4);
        sscanf(argv[3], "%3u.%3u.%3u.%3u", &b1, &b2, &b3, &b4);
        IP4_ADDR(&hisAddr, b1, b2, b3, b4);

    }
    else
    {
        printf("Settings not valid!\nExample: COM1 192.168.1.2 192.168.1.3\n");
        return -1;
    }

    waitPPPConnect(port, SERIAL_BAUD);

    size_t pos;
    if ((pos = port.find("COM", 0)) != string::npos)
    {
        string portNum = port.substr(pos + strlen("COM"), string::npos);
        sio_idx = stoi(portNum);
    }
    else
    {
        print_usage();
        return 1;
    }

//    if (argc > 1)
//    {
//        sio_idx = (u8_t) atoi(argv[1]);
//    }
    printf("Using serial port %d for PPP\n", sio_idx);

    /* no stdio-buffering, please! */
    setvbuf(stdout, NULL, _IONBF, 0);

    main_loop();

    return 0;
}
