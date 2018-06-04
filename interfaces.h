//
// Created by p.usas on 31.05.2018.
//
#pragma once

#include <string>

namespace interfaces
{
    struct param_t
    {
        u_long addr;
        u_long freeAddr;
        u_long mask;
        u_long gateway;
        u_long dns;
        std::string name;
        std::wstring friendlyName;
        param_t():addr(0), mask(0), gateway(0), dns(0)
        {
        }
    };

    int get(param_t& param, bool onlyLoopback = false);
    void createLoopback();

    void setLoopbackAddr(std::wstring name);


    void getFreeAddr(param_t& param);

    void createRoute(param_t param, std::string addr);
}
