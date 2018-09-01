#include <windows.h>
#include "napi.h"
using namespace Napi;

#define DRIVER_LENGTH 150

/*
 * Retrieve Windows Logical Drives (with Drive type & Free spaces).
 */
Array getLogicalDrives(const CallbackInfo& info) {
    Env env = info.Env();

    // Retrieve Logicial Drives
    TCHAR szBuffer[DRIVER_LENGTH];
    DWORD dwResult = GetLogicalDriveStrings(DRIVER_LENGTH, szBuffer);

    // Throw error if we fail to retrieve result
    if (dwResult == 0 || dwResult > DRIVER_LENGTH) {
        throw Error::New(env, "Failed to retrieve LogicalDrive!");
    }
    Array ret = Array::New(env);

    TCHAR *lpRootPathName = szBuffer;
    unsigned int i = 0;
    while (*lpRootPathName) {

        // Retrieve Drive Free Space
        DWORD dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;
        bool fResult = GetDiskFreeSpace(
            lpRootPathName,
            &dwSectPerClust,
            &dwBytesPerSect,
            &dwFreeClusters,
            &dwTotalClusters
        );

        if(fResult && dwBytesPerSect != 0 ){
            // Retrieve Drive Type
            UINT driveType = GetDriveType(lpRootPathName);

            // Convert to double and Calcule FreeClusterPourcent
            double FreeClusters = (double) dwFreeClusters;
            double TotalClusters = (double) dwTotalClusters;
            double FreeClusterPourcent = (FreeClusters / TotalClusters) * 100;

            // Setup JavaScript Object
            Object drive = Object::New(env);

            drive.Set("name", lpRootPathName);
            drive.Set("driveType", (double) driveType);
            drive.Set("bytesPerSect", (double) dwBytesPerSect);
            drive.Set("freeClusters", FreeClusters);
            drive.Set("totalClusters", TotalClusters);
            drive.Set("usedClusterPourcent", 100 - FreeClusterPourcent);
            drive.Set("freeClusterPourcent", FreeClusterPourcent);

            ret[i] = drive;
        }

        lpRootPathName += strlen((const char*) lpRootPathName) + 1;
        i++;
    }

    return ret;
}

// Initialize Native Addon
Object Init(Env env, Object exports) {
    // Setup methods
    exports.Set("getLogicalDrives", Function::New(env, getLogicalDrives));

    return exports;
}

// Export
NODE_API_MODULE(windisk, Init)
