#include "disktool.h"
#include <QDebug>

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))

DiskTool::DiskTool()
{
}

quint64 DiskTool::identifyFile(QString in){
    #if defined __linux__ || defined __APPLE__
        int suc = stat(in.toLocal8Bit().constData(), &info);
        if(!suc){
            return info.st_ino;
        }else{
            return 0;
        }
    #endif

    #ifdef _WIN32

        fileHandle = CreateFileA(in.toLocal8Bit().constData(),FILE_READ_ATTRIBUTES, FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_BACKUP_SEMANTICS,NULL);
        int suc = GetFileInformationByHandle(fileHandle, &lpFileInformation);
        CloseHandle(fileHandle);

        if(suc){
            return lpFileInformation.nFileIndexLow;
        }else{
            return 0;
        }


    #endif

}

quint64 DiskTool::identifyDisk(QString in){
    #if defined __linux__ || defined __APPLE__
        return 0;
    #endif

    #ifdef _WIN32

        in = in +"\\\\";

        WCHAR szVolumeName[256];
        WCHAR szFileSystemName[256];
        DWORD dwSerialNumber = 0;
        DWORD dwMaxFileNameLength=256;
        DWORD dwFileSystemFlags=0;

        bool rc = GetVolumeInformation( (WCHAR *) in.utf16(), szVolumeName, 256, &dwSerialNumber, &dwMaxFileNameLength, &dwFileSystemFlags, szFileSystemName, 256);

        if(!rc)
            return 0;
        else{
            qDebug()<<in<<" DRIVE ID "<<dwSerialNumber;
            return (quint64)dwSerialNumber;
        }

    #endif

}
