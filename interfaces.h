//
// Created by p.usas on 31.05.2018.
//
#pragma once

#include <string>
#include <vector>


namespace interfaces
{

    typedef std::vector<u_long> addrVector_t;
    typedef enum
    {
        NONE = 0,
        WIRELESS = 1,
        ETHERNET = 2,
        OTHER = 3,
    }type_t;


    struct param_t
    {
        type_t type;        // тип (для поиска)
        bool statusYA;      // статус интернета (для поиска)
        bool loopback;      // петля (для поиска)
        u_long addr;        // адрес интерфейса
        u_long mask;        // маска подесети интерфейса
        u_long gateway;     // шлюз по умолчанию
        u_long dns;         // перыый днс сервер
        bool dhcp;          // включен dhcp
        std::string name;   // уникальное название адаптера
        std::wstring friendlyName; // название адаптера
        param_t()
        {
            addr = 0;
            type = NONE;
            statusYA = false;
            loopback = false;
            addr = 0;
            mask = 0;
            gateway = 0;
            dns = 0;
            dhcp = false;
        }
    };

    bool get(param_t& param, bool onlyLoopback = false);
    void createLoopback();

    void setLoopbackAddr(std::wstring name);

    std::string decToStr(u_long addr);
    bool loopback(std::wstring description);
    type_t convertType(DWORD type);
    bool checkYA(u_long addr);
    bool ping(u_long addr);
    bool update(const param_t cur, param_t& lst);
    void createRoute(u_long addr, u_long gw);

    void getListArpAddrs(addrVector_t& addrs);
    u_long getFreeAddr(param_t& param);

}
