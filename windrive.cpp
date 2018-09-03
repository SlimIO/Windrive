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
 * 
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getlogicaldrivestringsw
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getdrivetypea
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getdiskfreespacea
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

            drive.Set("bytesPerSect", dwBytesPerSect);
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
 * 
 * Link to Microsoft documentation to understood how the code as been achieved:
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-createfilea
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/devio/calling-deviceiocontrol
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/winioctl/ni-winioctl-ioctl_disk_performance
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/winioctl/ns-winioctl-_disk_performance
 */
bool HandleDiskPerformance(LPCSTR wszPath, DISK_PERFORMANCE *pdg) {
    HANDLE hDevice = CreateFileA(
        wszPath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL
    );              

    // cannot open the drive
    if (hDevice == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD junk = 0;
    bool bResult = DeviceIoControl(
        hDevice, IOCTL_DISK_PERFORMANCE, NULL, 0, pdg, sizeof(*pdg), &junk, (LPOVERLAPPED) NULL
    );     
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

    // driveName should be typeof Napi::String
    if (!info[0].IsString()) {
        Error::New(env, "argument driveName should be typeof string!").ThrowAsJavaScriptException();
        return env.Null();
    }

    /*
     * Retrieve (Drive/PhysicalDrive) complete name as LPCSTR
     */
    string driveName = info[0].As<String>().Utf8Value();
    std::stringstream ss;
    ss << "\\\\.\\" << driveName; // Concat these weird carac (they are required to work).
    string tDriveName = ss.str();
    LPCSTR wszDrive = tDriveName.c_str();

    // Retrieve Typedef struct DISK_PERFORMANCE
    DISK_PERFORMANCE pdg = { 0 };
    bool bResult = HandleDiskPerformance(wszDrive, &pdg);
    if (!bResult) {
        Error::New(env, "Failed to retrieve drive performance!").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Transform WCHAR to _bstr_t (to be translated into a const char*)
    _bstr_t charStorageManagerName(pdg.StorageManagerName);

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
    ret.Set("storageManagerName", (const char*) charStorageManagerName);

    return ret;
}

/*
 * Retrieve Dos Devices
 */
Object getDosDevices(const CallbackInfo& info) {
    Env env = info.Env();
    Object ret = Object::New(env);

    // TODO: Find the right memory allocation ? (double api call ?).
    char logical[65536];
    char physical[65536];

    QueryDosDeviceA(NULL, physical, sizeof(physical));
    for (char *pos = physical; *pos; pos+=strlen(pos)+1) {
        QueryDosDeviceA(pos, logical, sizeof(logical));
        ret.Set(pos, logical);
    }    

    return ret;
}

// Initialize Native Addon
Object Init(Env env, Object exports) {

    // Setup methods
    // TODO: Launch with AsyncWorker to avoid event loop starvation
    exports.Set("getLogicalDrives", Function::New(env, getLogicalDrives));
    exports.Set("getDevicePerformance", Function::New(env, getDevicePerformance));
    exports.Set("getDosDevices", Function::New(env, getDosDevices));

    return exports;
}

// Export
NODE_API_MODULE(windrive, Init)
