#include <windows.h>
#include <comdef.h>
#include <sstream>
#include <string>
#include "napi.h"
using namespace std;
using namespace Napi;

/*
 * Buffer length for logical drives names
 * TODO: Best value ?
 */
#define DRIVER_LENGTH 150

/*
 * Retrieve Windows Logical Drives (with Drive type & Free spaces).
 */
Value getLogicalDrives(const CallbackInfo& info) {
    Env env = info.Env();

    // Retrieve Logicial Drives
    TCHAR szBuffer[DRIVER_LENGTH];
    DWORD dwResult = GetLogicalDriveStrings(DRIVER_LENGTH, szBuffer);

    // Throw error if we fail to retrieve result
    if (dwResult == 0 || dwResult > DRIVER_LENGTH) {
        Error::New(env, "Failed to retrieve logical drives names!").ThrowAsJavaScriptException();

        return env.Null();
    }

    // TODO: Find Array size with dwResult ?
    Array ret = Array::New(env);

    TCHAR *lpRootPathName = szBuffer;
    unsigned int i = 0;
    while (*lpRootPathName) {

        // Setup JavaScript Object
        Object drive = Object::New(env);
        ret[i] = drive;

        drive.Set("name", lpRootPathName);

        // Retrieve Drive Type
        UINT driveType = GetDriveType(lpRootPathName);

        drive.Set("driveType", (double) driveType);

        // Retrieve Drive Free Space
        DWORD dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;
        bool fResult = GetDiskFreeSpace(
            lpRootPathName,
            &dwSectPerClust,
            &dwBytesPerSect,
            &dwFreeClusters,
            &dwTotalClusters
        );

        if(fResult && dwBytesPerSect != 0) {
            // Convert to double and Calcule FreeClusterPourcent
            double FreeClusters = (double) dwFreeClusters;
            double TotalClusters = (double) dwTotalClusters;
            double FreeClusterPourcent = (FreeClusters / TotalClusters) * 100;

            drive.Set("bytesPerSect", (double) dwBytesPerSect);
            drive.Set("freeClusters", FreeClusters);
            drive.Set("totalClusters", TotalClusters);
            drive.Set("usedClusterPourcent", 100 - FreeClusterPourcent);
            drive.Set("freeClusterPourcent", FreeClusterPourcent);
        }

        lpRootPathName += strlen((const char*) lpRootPathName) + 1;
        i++;
    }

    return ret;
}

/*
 * Retrieve Disk Performance
 */
bool GetDiskPerformance(LPCSTR wszPath, DISK_PERFORMANCE *pdg) {
    HANDLE hDevice = INVALID_HANDLE_VALUE;  // handle to the drive to be examined 
    bool bResult   = FALSE;                 // results flag
    DWORD junk     = 0;                     // discard results

    hDevice = CreateFileA(wszPath,          // drive to open
                        0,                  // no access to the drive
                        FILE_SHARE_READ |   // share mode
                        FILE_SHARE_WRITE, 
                        NULL,               // default security attributes
                        OPEN_EXISTING,      // disposition
                        0,                  // file attributes
                        NULL);              // do not copy file attributes

    // cannot open the drive
    if (hDevice == INVALID_HANDLE_VALUE) {
        return false;
    }

    bResult = DeviceIoControl(hDevice,               // device to be queried
                        IOCTL_DISK_PERFORMANCE,      // operation to perform
                        NULL, 0,                     // no input buffer
                        pdg, sizeof(*pdg),           // output buffer
                        &junk,                       // # bytes returned
                        (LPOVERLAPPED) NULL);        // synchronous I/O
    CloseHandle(hDevice);

    return bResult;
}

/*
 * Binding for retrieving drive performance
 */
Value getDevicePerformance(const CallbackInfo& info) {
    Env env = info.Env();

    // Check argument length!
    if (info.Length() < 1) {
        Error::New(env, "Wrong number of argument provided!").ThrowAsJavaScriptException();

        return env.Null();
    }

    // DriveName should be typeof string
    if (!info[0].IsString()) {
        Error::New(env, "argument driveName should be typeof string!").ThrowAsJavaScriptException();

        return env.Null();
    }

    // Retrieve Device complete name!
    string driveName = info[0].As<String>().Utf8Value();
    std::stringstream ss;
    ss << "\\\\.\\" << driveName;
    string tDriveName = ss.str();
    LPCSTR wszDrive = tDriveName.c_str();

    DISK_PERFORMANCE pdg = { 0 };

    // Get Disk Performance
    bool bResult = GetDiskPerformance(wszDrive, &pdg);
    if (!bResult) {
        Error::New(env, "Failed to retrieve drive performance !").ThrowAsJavaScriptException();

        return env.Null();
    }

    // Setup JavaScript Object
    Object ret = Object::New(env);
    ret.Set("bytesRead", pdg.BytesRead.QuadPart);
    ret.Set("bytesWritten", pdg.BytesWritten.QuadPart);
    ret.Set("readTime", pdg.ReadTime.QuadPart);
    ret.Set("writeTime", pdg.WriteTime.QuadPart);
    ret.Set("idleTime", pdg.IdleTime.QuadPart);
    ret.Set("readCount", pdg.ReadCount);
    ret.Set("writeCount", pdg.WriteCount);
    ret.Set("queueDepth", pdg.QueueDepth);
    ret.Set("splitCount", pdg.SplitCount);
    ret.Set("queryTime", pdg.QueryTime.QuadPart);
    ret.Set("storageDeviceNumber", pdg.StorageDeviceNumber);
    _bstr_t charStorageManagerName(pdg.StorageManagerName);
    ret.Set("storageManagerName", (const char*) charStorageManagerName);

    return ret;
}

// Initialize Native Addon
Object Init(Env env, Object exports) {

    // Setup methods
    // TODO: Launch with AsyncWorker to avoid event loop starvation
    exports.Set("getLogicalDrives", Function::New(env, getLogicalDrives));
    exports.Set("getDevicePerformance", Function::New(env, getDevicePerformance));

    return exports;
}

// Export
NODE_API_MODULE(windisk, Init)
