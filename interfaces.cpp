//
// Created by p.usas on 31.05.2018.
//

#include <tchar.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <ws2tcpip.h>
#include <iostream>
#include <inaddr.h>
#include <algorithm>
#include "interfaces.h"

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

namespace interfaces
{
    bool get(param_t &param, bool onlyLoopback)
    {
        printf("get\n");
        param_t lstParam;
        DWORD dwRetVal = 0;

        ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_INCLUDE_GATEWAYS;
        ULONG family = AF_INET;
        ULONG outBufLen = 0;
        PIP_ADAPTER_ADDRESSES pAddresses = NULL;
        dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
        if (dwRetVal == ERROR_BUFFER_OVERFLOW)
        {
            pAddresses = (PIP_ADAPTER_ADDRESSES) MALLOC(outBufLen);
            if (pAddresses == NULL)
            {
                printf("Unable to allocate memory needed to call GetInterfaceInfo\n");
                return false;
            }
        }
        else
        {
            return false;
        }
        dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
        if (dwRetVal == NO_ERROR && pAddresses)
        {
            for (PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses; pCurrAddresses; pCurrAddresses = pCurrAddresses->Next)
            {
                if (pCurrAddresses->OperStatus == 1)
                {
                    param_t curParam;
                    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress;
                    PIP_ADAPTER_GATEWAY_ADDRESS pGWaddr = pCurrAddresses->FirstGatewayAddress;
                    PIP_ADAPTER_DNS_SERVER_ADDRESS pDnsaddr = pCurrAddresses->FirstDnsServerAddress;

                    curParam.name = pCurrAddresses->AdapterName;
                    curParam.dhcp = (pCurrAddresses->Flags & IP_ADAPTER_DHCP_ENABLED) != 0;

                    if (pUnicast)
                    {
                        curParam.addr = ((sockaddr_in *) pUnicast->Address.lpSockaddr)->sin_addr.s_addr;
                        ConvertLengthToIpv4Mask(pUnicast->OnLinkPrefixLength, &curParam.mask);
                    }
                    decToStr(curParam.addr);
                    if (pGWaddr)
                    {
                        curParam.gateway = ((sockaddr_in *) pGWaddr->Address.lpSockaddr)->sin_addr.s_addr;
                    }
                    if (pDnsaddr)
                    {
                        curParam.dns = ((sockaddr_in *) pDnsaddr->Address.lpSockaddr)->sin_addr.s_addr;
                    }

                    curParam.friendlyName = pCurrAddresses->FriendlyName;
                    curParam.type = convertType(pCurrAddresses->IfType);
                    curParam.statusYA = checkYA(curParam.addr);

                    std::wstring description(pCurrAddresses->Description);

                    if (onlyLoopback)
                    {
                        if (description.find(L"Loopback") != std::string::npos && curParam.type == ETHERNET)
                        {
                            lstParam = curParam;
                            break;
                        }
                        else
                        {
                            continue;
                        }
                    }
                    else
                    {
                        if (description.find(L"Loopback") != std::string::npos && curParam.type == ETHERNET)
                        {
                            continue;
                        }
                        else if (update(curParam, lstParam))
                        {
                            break;
                        }

                    }

//                    if (description.find(L"Loopback") != std::string::npos && curParam.type == ETHERNET)
//                    {
//                        if (onlyLoopback)
//                        {
//                            lstParam = curParam;
//                            break;
//                        }
//                        else
//                        {
//                            continue;
//                        }
//
//                    }

//                    else if (update(curParam, lstParam))
//                    {
//                        break;
//                    }

                }
                else
                {
                    continue;
                }
            }
        }
        else
        {
            return false;
        }

        if (pAddresses)
        {
            FREE(pAddresses);
        }

        param = lstParam;
        return param.type != NONE;
    }

    void createLoopback()
    {
        BOOL Wow64Flag = FALSE;
        TCHAR CmdName[1024];
        if (IsWow64Process(GetCurrentProcess(), &Wow64Flag) && Wow64Flag)
        {
            lstrcpy(CmdName, _T("lbadpt64.exe install"));
        }
        else
        {
            lstrcpy(CmdName, _T("lbadpt32.exe install"));
        }

        PROCESS_INFORMATION pi;
        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        if (CreateProcess(NULL, CmdName, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi) == 0)
        {
            printf("createLoopback CreateProcess error\n");
            return;
        }
        DWORD RetCode;
        do
        {
            Sleep(500);
            GetExitCodeProcess(pi.hProcess, &RetCode);
        }
        while (RetCode == STILL_ACTIVE);
        param_t param;
        if (get(param, true))
        {
            setLoopbackAddr(param.friendlyName);
        }
        printf("createLoopback need restart\n");
    }

    void setLoopbackAddr(std::wstring name)
    {
        std::wstring cmdParam =
                L"/C netsh interface ip set address \"" + name + L"\" static 192.168.1.1 255.255.255.0 192.168.1.254";
        ShellExecuteW(NULL, L"open", L"cmd.exe", cmdParam.c_str(), NULL, SW_HIDE);
    }

    std::string decToStr(u_long addr)
    {
        struct in_addr tmp;
        tmp.S_un.S_addr = addr;
        return std::string(inet_ntoa(tmp));
    }

    bool checkYA(u_long addr)
    {
        std::string cmd = "ping ya.ru -S " + decToStr(addr) + " -n 1 -w 2000";
        return system(cmd.c_str()) == 0;
    }

    bool ping(u_long addr)
    {
        std::string cmd = "ping " + decToStr(addr) + " -n 1 -w 500";
        return system(cmd.c_str()) == 0;
    }

    type_t convertType(DWORD type)
    {
        switch (type)
        {
            case IF_TYPE_ETHERNET_CSMACD:
                return ETHERNET;
            case IF_TYPE_IEEE80211:
                return WIRELESS;
            default:
                return NONE;
        }
    }

    bool update(const param_t cur, param_t &lst)
    {
        if ((cur.statusYA && cur.type > lst.type) ||        // рабочий яндекс и приоритет выше
            (cur.statusYA && !lst.statusYA) ||              // рабочий и нерабочий
            (cur.type > lst.type && !lst.statusYA))         // нерабочий и приоритет выше
        {
            lst = cur;
        }
        return cur.type == ETHERNET && cur.statusYA;
    }

    void getListArpAddrs(addrVector_t &addrs)
    {
        unsigned long NetTableSize = 0;
        unsigned long ipres = GetIpNetTable(NULL, &NetTableSize, 0);

        if (ipres == ERROR_INSUFFICIENT_BUFFER && NetTableSize > 0)
        {
            PMIB_IPNETTABLE NetTable = new MIB_IPNETTABLE[NetTableSize];
            ipres = GetIpNetTable(NetTable, &NetTableSize, 0);

            if (ipres == 0 && NetTable->dwNumEntries > 0)
            {
                for (unsigned long i = 0; i < NetTable->dwNumEntries; i++)
                {
                    if (NetTable->table[i].dwType != MIB_IPNET_TYPE_INVALID)
                    {
                        addrs.push_back(NetTable->table[i].dwAddr);
                    }

                }
            }
            delete[] NetTable;
        }
    }

    u_long getFreeAddr(param_t &param)
    {
        addrVector_t addrs;
        getListArpAddrs(addrs);

        u_long first_ip = ntohl(param.addr & param.mask);
        u_long last_ip = ntohl(param.addr | ~(param.mask));

        for (u_long i = last_ip - 1; i > first_ip; i++)
        {
            if (std::find(addrs.begin(), addrs.end(), i) == addrs.end() && !ping(i))
            {
                return htonl(i);
            }
        }

        return 0;
    }
}

void createRoute(u_long addr, u_long gw)
{
    std::string strAddr = interfaces::decToStr(addr);
    std::string strGW = interfaces::decToStr(gw);
    {
        std::wstring cmdParam =
                L"/C route delete " + std::wstring(strAddr.begin(), strAddr.end()) + L" mask 255.255.255.255 ";
        ShellExecuteW(NULL, L"open", L"cmd.exe", cmdParam.c_str(), NULL, SW_HIDE);
        Sleep(5000);
    }

    {
        std::wstring cmdParam =
                L"/C route add " + std::wstring(strAddr.begin(), strAddr.end()) + L" mask 255.255.255.255 " +
                std::wstring(strGW.begin(), strGW.end());
        ShellExecuteW(NULL, L"open", L"cmd.exe", cmdParam.c_str(), NULL, SW_HIDE);
    }
}