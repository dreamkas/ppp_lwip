#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Генерирует случайный MAC-адрес
 * @param addr массив для записи адреса
 */
void GenerateMacAddress(uint8_t *addr);

#ifdef __cplusplus
}
#endif
