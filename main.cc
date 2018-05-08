//#include <w32api/vss.h>
#include <iostream>

// OS Specific sleep
#ifdef _WIN32

#include <windows.h>
#include <lwip/ip6.h>

#else
#include <unistd.h>
#endif

#include "serial/serial.h"

bool isPPPConnected;
extern u8_t sio_idx;

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

int run(int argc, char **argv)
{
    if (argc < 2)
    {
        print_usage();
        return 0;
    }

    // Argument 1 is the serial port or enumerate flag
    std::string port(argv[1]);

    if (port == "-e")
    {
        enumerate_ports();
        return 0;
    }
    else if (argc < 3)
    {
        print_usage();
        return 1;
    }

    // Argument 2 is the baudrate
    unsigned long baud = 0;
#if defined(WIN32) && !defined(__MINGW32__)
    sscanf_s(argv[2], "%lu", &baud);
#else
    sscanf(argv[2], "%lu", &baud);
#endif

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

int waitPPPConnect(int argc, char **argv)
{
    if (argc < 2)
    {
        print_usage();
        return 0;
    }

    // Argument 1 is the serial port or enumerate flag
    string port(argv[1]);

    if (port == "-e")
    {
        enumerate_ports();
        return 0;
    }
    else if (argc < 3)
    {
        print_usage();
        return 1;
    }

    // Argument 2 is the baudrate
    unsigned long baud = 0;
#if defined(WIN32) && !defined(__MINGW32__)
    sscanf_s(argv[2], "%lu", &baud);
#else
    sscanf(argv[2], "%lu", &baud);
#endif

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

extern "C" void main_loop();


int main(int argc, char **argv)
{
    waitPPPConnect(argc, argv);

    string port(argv[1]);
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
