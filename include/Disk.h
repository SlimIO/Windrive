#ifndef DISK_H
#define DISK_H

#include "afxcoll.h"
#include <windows.h>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
using namespace std;

#define LogicalDriverLength 1024

typedef struct diskInfo {
    char* Name;
    double FreeClusters;
    double TotalClusters;
    double UsedClusterPourcent;
    double FreeClusterPourcent;
    double DriveType;
} DiskInfo;

class Disk
{
    public:
        Disk();
        vector<diskInfo> getAllDiskInformation();

};

#endif // DISK_H
