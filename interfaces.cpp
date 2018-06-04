//
// Created by p.usas on 31.05.2018.
//

#include <tchar.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <ws2tcpip.h>
#include <iostream>
#include "interfaces.h"

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))
namespace interfaces
{
    int get(param_t &param, bool onlyLoopback)
    {
        printf("get\n");
        BOOL find = FALSE;
        DWORD dwSize = 0;
        DWORD dwRetVal = 0;

        // Set the flags to pass to GetAdaptersAddresses
        ULONG flags = GAA_FLAG_INCLUDE_PREFIX;

        // default to unspecified address family (both)
        ULONG family = AF_INET;
        PIP_ADAPTER_ADDRESSES pAddresses = NULL;
        ULONG outBufLen = 0;

        PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
        PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
        PIP_ADAPTER_GATEWAY_ADDRESS pGWaddr = NULL;
        PIP_ADAPTER_DNS_SERVER_ADDRESS pDnsaddr = NULL;

        dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
        if (dwRetVal == ERROR_BUFFER_OVERFLOW)
        {
            pAddresses = (PIP_ADAPTER_ADDRESSES) MALLOC(outBufLen);
            if (pAddresses == NULL)
            {
                printf("Unable to allocate memory needed to call GetInterfaceInfo\n");
                return -1;
            }
        }
        // Make a second call to GetInterfaceInfo to get
        // the actual data we need
        dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
        if (dwRetVal == NO_ERROR && pAddresses)
        {

            for (pCurrAddresses = pAddresses; pCurrAddresses; pCurrAddresses = pCurrAddresses->Next)
            {
                if (pCurrAddresses->OperStatus == 1 &&
                    (pCurrAddresses->IfType == 6 || pCurrAddresses->IfType == 71 ) &&
                    pCurrAddresses->FirstUnicastAddress)
                {
                    pUnicast = pCurrAddresses->FirstUnicastAddress;
                    pGWaddr = pCurrAddresses->FirstGatewayAddress;
                    pDnsaddr = pCurrAddresses->FirstDnsServerAddress;
                    param.name = pCurrAddresses->AdapterName;
//                    param.addr = ((sockaddr_in*)pUnicast->Address.lpSockaddr)->sin_addr.s_addr;
                    if (pGWaddr)
                    {
                        param.gateway = ((sockaddr_in *) pGWaddr->Address.lpSockaddr)->sin_addr.s_addr;
                    }
                    if (pDnsaddr)
                    {
                        param.dns = ((sockaddr_in *) pDnsaddr->Address.lpSockaddr)->sin_addr.s_addr;
                    }
                    ConvertLengthToIpv4Mask(pUnicast->OnLinkPrefixLength, &param.mask);
                    param.friendlyName = pCurrAddresses->FriendlyName;

                    std::wstring description = pCurrAddresses->Description;

                    if (onlyLoopback)
                    {
                        if (description.find(L"Loopback") != std::string::npos)
                        {
                            FREE(pAddresses);
                            return 0;
                        }
                    } else
                    {
                        find = TRUE;
                        if (description.find(L"Loopback") == std::string::npos)
                        {
                            if ( pCurrAddresses->IfType == 71)
                                param.addr = 3086657728; //183.250.168.192
                            else
                                param.addr = 3086133440; //192.168.242.183
                            FREE(pAddresses);
                            return 0;
                        }
                        else if (param.addr == 0)
                        {

                            param.addr = 1677830336; // 192.168.1.100
                        }
                    }
                } else {
                    continue;
                }
            }
        } else
        {
            printf("get dwRetVal\n");
            return -1;
        }

        if (pAddresses)
        {
            FREE(pAddresses);
        }
        if (find == TRUE)
        {
            return 0;
        }
        else
        {
            printf("(find == false\n");
            return -1;
        }
    }


    typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);

    void createLoopback()
    {
        BOOL Wow64Flag = FALSE;
        LPFN_ISWOW64PROCESS FnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(GetModuleHandle(_T("kernel32")),
                                                                                    "IsWow64Process");
        TCHAR CmdName[1024];

        char buffer[MAX_PATH];
        GetCurrentDirectory(MAX_PATH, buffer );

        lstrcpy(CmdName, _T(buffer));

        if (NULL != FnIsWow64Process && !FnIsWow64Process(GetCurrentProcess(), &Wow64Flag))
        {
            Wow64Flag = FALSE;
        }


        if (Wow64Flag)
        {
            lstrcat(CmdName, _T("\\lbadpt64.exe "));
        } else
        {
            lstrcat(CmdName, _T("\\lbadpt32.exe "));
        }

        lstrcat(CmdName, _T("install"));

        PROCESS_INFORMATION pi;
        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        if (CreateProcess(NULL, CmdName, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi) == 0)
        {
            printf("createLoopback CreateProcess error\n" );
            return;
        }
        DWORD RetCode;
        do
        {
            Sleep(500);
            GetExitCodeProcess(pi.hProcess, &RetCode);
        } while (RetCode == STILL_ACTIVE);
        param_t param;
        if (get(param, true) == 0)
        {
            setLoopbackAddr(param.friendlyName);
        }
        printf("createLoopback need restart\n" );
    }

    void setLoopbackAddr(std::wstring name)
    {
        std::wstring cmdParam =
                L"/K netsh interface ip set address " + name + L" static 192.168.1.1 255.255.255.0 192.168.1.100 1";
        ShellExecuteW(NULL, L"open", L"cmd.exe", cmdParam.c_str(), NULL, SW_SHOWNORMAL);
    }

    void createRoute(param_t param, std::string addr)
    {
        printf("createRoute addr %s\n", addr.c_str());
        {
            std::wstring cmdParam = L"/K route delete " + std::wstring(addr.begin(), addr.end()) + L" mask 255.255.255.255 ";
            ShellExecuteW(NULL, L"open", L"cmd.exe", cmdParam.c_str(), NULL, SW_SHOWNORMAL);
            Sleep(5000);
        }
        if (param.addr == 3086133440) //192.168.242.183
        {
            std::wstring cmdParam = L"/K route add " + std::wstring(addr.begin(), addr.end()) + L" mask 255.255.255.255 192.168.242.183";
            ShellExecuteW(NULL, L"open", L"cmd.exe", cmdParam.c_str(), NULL, SW_SHOWNORMAL);
        }
        else if (param.addr == 3086657728) // 192.168.250.183)
        {
            printf("createRoute addr 3086657728\n");
            std::wstring cmdParam = L"/K route add " + std::wstring(addr.begin(), addr.end()) + L" mask 255.255.255.255 192.168.250.183";
            ShellExecuteW(NULL, L"open", L"cmd.exe", cmdParam.c_str(), NULL, SW_SHOWNORMAL);
        }
        else if (param.addr == 1677830336) // 192.168.1.100)
        {
            std::wstring cmdParam = L"/K route add " + std::wstring(addr.begin(), addr.end()) + L" mask 255.255.255.255 192.168.1.100";
            ShellExecuteW(NULL, L"open", L"cmd.exe", cmdParam.c_str(), NULL, SW_SHOWNORMAL);
        }
    }
}