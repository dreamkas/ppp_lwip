#include "mac_generator.h"
#include "lwip/include/lwip/prot/ethernet.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void GenerateMacAddress(uint8_t *addr)
{
    srand(time(NULL) + getpid());

    for (int i = 0; i < ETH_HWADDR_LEN; i++)
    {
        addr[i] = (uint8_t)(rand() % 256);
    }

    addr[0] &= ~0x1; // снимаем бит группового адреса
}
