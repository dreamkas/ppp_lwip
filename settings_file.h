#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Инициализация работы с файлом настроек, должна быть вызвана перед использование других функций
 * @param fileName имя файла
 * @return true - успешно, false - ошибка
 */
bool SettingsFile_Init(const char *fileName);

/**
 * Установить параметр
 * @param name имя параметра
 * @param value значение параметра
 * @return true - успешно, false - ошибка
 */
bool SettingsFile_Set(const char *name, const char *value);

/**
 * Получить параметр из файла
 * @param name имя параметра
 * @param value массив для записи значения параметра
 * @return true - успешно, false - ошибка
 */
bool SettingsFile_Get(const char *name, char *value);

#ifdef __cplusplus
}
#endif
