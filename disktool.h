#ifndef DISKTOOL_H
#define DISKTOOL_H

#if defined __linux__ || defined __APPLE__
#include<sys/stat.h>
#endif
#ifdef _WIN32
#include <windows.h>
#include <WinIoCtl.h>
#endif
#include <QString>

class DiskTool
{
public:
    DiskTool();
    quint64 identifyFile(QString);
    quint64 identifyDisk(QString);

private:
#if defined __linux__ || defined __APPLE__
    struct stat info;
#endif

#ifdef _WIN32
    BY_HANDLE_FILE_INFORMATION lpFileInformation;
    HANDLE fileHandle;
#endif
};

#endif
