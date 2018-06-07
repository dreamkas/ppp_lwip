#include "settings_file.h"
#include <string>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>

using namespace std;

static string fileName;

static bool init(const char *name)
{
    try
    {
        fileName = name;
        fstream file;
        file.open(fileName, ios::app);
    }
    catch(...)
    {
        cout << "Error creating filename string" << endl;
        return false;
    }

    return true;
}

static bool get(const char *name, char *value)
{
    try
    {
        string paramName(name);
        string line;
        ifstream file(fileName);

        if (!file)
        {
            cout << "Error opening file" << endl;
            return false;
        }

        while (file >> line)
        {
            if (line.find(paramName + "=") == 0)
            {
                strcpy(value, line.substr(paramName.length() + 1).c_str()); // +1 для разделяющего знака
                return true;
            }
        }

        cout << "Parameter " << paramName << " not found in " << fileName << endl;
        return false;
    }
    catch(...)
    {
        return false;
    }
}

static bool set(const char *name, const char *value)
{
    const string copyPostfix(".tmp");

    try
    {
        string paramName(name);
        string paramValue(value);
        string line;
        ifstream inputFile(fileName);
        ofstream outputFile(fileName + copyPostfix);

        if (!inputFile || !outputFile)
        {
            cout << "Error opening file" << endl;
            return false;
        }

        bool paramFound = false;

        while (inputFile >> line)
        {
            if (line.find(paramName + "=") == 0)
            {
                line = paramName;
                line.append("=");
                line.append(paramValue);
                paramFound = true;
            }

            outputFile << line << endl;
        }

        if (!paramFound)
        {
            outputFile << paramName << "=" << paramValue << endl;
        }

        inputFile.close();
        outputFile.close();
        remove(fileName.c_str());
        rename((fileName + copyPostfix).c_str(), fileName.c_str());
        return true;
    }
    catch(...)
    {
        return false;
    }
}


extern "C"
{

bool SettingsFile_Init(const char *fileName)
{
    return init(fileName);
}

bool SettingsFile_Set(const char *name, const char *value)
{
    return set(name, value);
}

bool SettingsFile_Get(const char *name, char *value)
{
    return get(name, value);

}

}
