#include <windows.h>
#include <comdef.h>
#include <sstream>
#include <string>
#include "napi.h"

using namespace std;
using namespace Napi;

/*
 * Buffer length for logical drives names
 * TODO: Best value ? (120 is almost good for 30 disks)
 */
#define DRIVER_LENGTH 120

/*
 * Logical Drive struct
 */
struct LogicalDrive {
    TCHAR* name;
    string driveType;
    DWORD bytesPerSect;
    DWORD freeClusters;
    DWORD totalClusters;
    double usedClusterPourcent;
    double freeClusterPourcent;
};

/*
 * Disk Performance struct
 */
struct DiskPerformance {
    LONGLONG bytesRead;
    LONGLONG bytesWritten;
    LONGLONG readTime;
    LONGLONG writeTime;
    LONGLONG idleTime;
    DWORD readCount;
    DWORD writeCount;
    DWORD queueDepth;
    DWORD splitCount;
    LONGLONG queryTime;
    DWORD storageDeviceNumber;
    const char* storageManagerName;
};

/*
 * Asycnronous Worker to Retrieve Windows Logical Drives
 * 
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getlogicaldrivestringsw
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getdrivetypea
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getdiskfreespacea
 */
class LogicalDriveWorker : public AsyncWorker {
    public:
        LogicalDriveWorker(Function& callback) : AsyncWorker(callback) {}
        ~LogicalDriveWorker() {}

    private:
        vector<LogicalDrive> vLogicalDrives;

    // This code will be executed on the worker thread
    void Execute() {
        BOOL success;
        UINT driveType;
        TCHAR szBuffer[DRIVER_LENGTH];
        DWORD dwResult = GetLogicalDriveStrings(DRIVER_LENGTH, szBuffer);

        // Throw error if we fail to retrieve result
        if (dwResult == 0 || dwResult > DRIVER_LENGTH) {
            return SetError("Failed to retrieve logical drives names!");
        }

        TCHAR *lpRootPathName = szBuffer;
        while (*lpRootPathName) {

            // Instanciate Variables
            DWORD dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;
            LogicalDrive drive;

            // Setup default variables!
            drive.name = lpRootPathName;
            driveType = GetDriveType(lpRootPathName);
            switch(driveType) {
                case DRIVE_UNKNOWN:
                    drive.driveType = string("UNKNOWN");
                    break;
                case DRIVE_NO_ROOT_DIR:
                    drive.driveType = string("NO_ROOT_DIR");
                    break;
                case DRIVE_REMOVABLE:
                    drive.driveType = string("REMOVABLE");
                    break;
                case DRIVE_FIXED:
                    drive.driveType = string("FIXED");
                    break;
                case DRIVE_REMOTE:
                    drive.driveType = string("REMOTE");
                    break;
                case DRIVE_CDROM:
                    drive.driveType = string("CDROM");
                    break;
                case DRIVE_RAMDISK:
                    drive.driveType = string("RAMDISK");
                    break;
            }

            // Ignore CDROM, they have no Space or anything!
            if (driveType != DRIVE_CDROM) {
                // Retrieve Drive Free Space
                success = GetDiskFreeSpace(
                    lpRootPathName,
                    &dwSectPerClust,
                    &dwBytesPerSect,
                    &dwFreeClusters,
                    &dwTotalClusters
                );

                if(success && dwBytesPerSect != 0) {
                    double FreeClusterPourcent = (dwFreeClusters / dwTotalClusters) * 100;

                    drive.bytesPerSect = dwBytesPerSect;
                    drive.freeClusters = dwFreeClusters;
                    drive.totalClusters = dwTotalClusters;
                    drive.usedClusterPourcent = 100 - FreeClusterPourcent;
                    drive.freeClusterPourcent = FreeClusterPourcent;
                }
            }

            vLogicalDrives.push_back(drive);
            lpRootPathName += strlen((const char*) lpRootPathName) + 1;
        }
    }

    void OnOK() {
        HandleScope scope(Env());
        Array ret = Array::New(Env());
        for (size_t i = 0; i < vLogicalDrives.size(); ++i) {
            LogicalDrive currDrive = vLogicalDrives.at(i);
            Object currJSDrive = Object::New(Env());
            ret[i] = currJSDrive;

            currJSDrive.Set("name", currDrive.name);
            currJSDrive.Set("type", currDrive.driveType);
            currJSDrive.Set("bytesPerSect", currDrive.bytesPerSect);
            currJSDrive.Set("freeClusters", currDrive.freeClusters);
            currJSDrive.Set("totalClusters", currDrive.totalClusters);
            currJSDrive.Set("usedClusterPourcent", currDrive.usedClusterPourcent);
            currJSDrive.Set("freeClusterPourcent", currDrive.freeClusterPourcent);
        }
        Callback().Call({Env().Null(), ret});
    }

};

/*
 * Retrieve Windows Logical Drives (with Drive type & Free spaces).
 */
Value getLogicalDrives(const CallbackInfo& info) {
    Env env = info.Env();

    // Check argument length!
    if (info.Length() < 1) {
        Error::New(env, "Wrong number of argument provided!").ThrowAsJavaScriptException();
        return env.Null();
    }

    // driveName should be typeof Napi::String
    if (!info[0].IsFunction()) {
        Error::New(env, "argument callback should be a Function!").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Execute work with callback!
    Function cb = info[0].As<Function>();
    (new LogicalDriveWorker(cb))->Queue();

    return env.Undefined();
}

/*
 * Retrieve Disk Performance Worker
 * 
 * Link to Microsoft documentation to understood how the code as been achieved:
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-createfilea
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/devio/calling-deviceiocontrol
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/winioctl/ni-winioctl-ioctl_disk_performance
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/winioctl/ns-winioctl-_disk_performance
 */
class DiskPerformanceWorker : public AsyncWorker {
    public:
        DiskPerformanceWorker(Function& callback, string driveName) : AsyncWorker(callback), driveName(driveName) {}
        ~DiskPerformanceWorker() {}

    private:
        string driveName;
        DiskPerformance sDiskPerformance;

    // This code will be executed on the worker thread
    void Execute() {
        stringstream ss;
        ss << "\\\\.\\" << driveName; // Concat these weird carac (they are required to work).
        string tDriveName = ss.str();
        LPCSTR wszDrive = tDriveName.c_str();

        // Retrieve Typedef struct DISK_PERFORMANCE
        DISK_PERFORMANCE pdg = { 0 };

        HANDLE hDevice = CreateFileA(
            wszDrive, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL
        );              

        // cannot open the drive
        if (hDevice == INVALID_HANDLE_VALUE) {
            return SetError("Invalid Handle value for indicated Drive!");
        }

        DWORD junk = 0;
        bool bResult = DeviceIoControl(
            hDevice, IOCTL_DISK_PERFORMANCE, NULL, 0, &pdg, sizeof(pdg), &junk, (LPOVERLAPPED) NULL
        );     
        CloseHandle(hDevice);

        // Transform WCHAR to _bstr_t (to be translated into a const char*)
        _bstr_t charStorageManagerName(pdg.StorageManagerName);

        sDiskPerformance.bytesRead = pdg.BytesRead.QuadPart;
        sDiskPerformance.bytesWritten = pdg.BytesWritten.QuadPart;
        sDiskPerformance.readTime = pdg.ReadTime.QuadPart;
        sDiskPerformance.writeTime = pdg.WriteTime.QuadPart;
        sDiskPerformance.idleTime = pdg.IdleTime.QuadPart;
        sDiskPerformance.readCount = pdg.ReadCount;
        sDiskPerformance.writeCount = pdg.WriteCount;
        sDiskPerformance.queueDepth = pdg.QueueDepth;
        sDiskPerformance.splitCount = pdg.SplitCount;
        sDiskPerformance.queryTime = pdg.QueryTime.QuadPart;
        sDiskPerformance.storageDeviceNumber = pdg.StorageDeviceNumber;
        sDiskPerformance.storageManagerName = (const char*) charStorageManagerName;
    }

    void OnOK() {
        HandleScope scope(Env());
        Object ret = Object::New(Env());
        ret.Set("bytesRead", sDiskPerformance.bytesRead);
        ret.Set("bytesWritten", sDiskPerformance.bytesWritten);
        ret.Set("readTime", sDiskPerformance.readTime);
        ret.Set("writeTime", sDiskPerformance.writeTime);
        ret.Set("idleTime", sDiskPerformance.idleTime);
        ret.Set("readCount", sDiskPerformance.readCount);
        ret.Set("writeCount", sDiskPerformance.writeCount);
        ret.Set("queueDepth", sDiskPerformance.queueDepth);
        ret.Set("splitCount", sDiskPerformance.splitCount);
        ret.Set("queryTime", sDiskPerformance.queryTime);
        ret.Set("storageDeviceNumber", sDiskPerformance.storageDeviceNumber);
        ret.Set("storageManagerName", sDiskPerformance.storageManagerName);

        Callback().Call({Env().Null(), ret});
    }

};

/*
 * Binding for retrieving drive performance
 */
Value getDevicePerformance(const CallbackInfo& info) {
    Env env = info.Env();

    // Check argument length!
    if (info.Length() < 2) {
        Error::New(env, "Wrong number of argument provided!").ThrowAsJavaScriptException();
        return env.Null();
    }

    // callback should be typeof Napi::String
    if (!info[1].IsFunction()) {
        Error::New(env, "argument callback should be a Function!").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Execute worker thread!
    string driveName = info[0].As<String>().Utf8Value();
    Function cb = info[1].As<Function>();
    (new DiskPerformanceWorker(cb, driveName))->Queue();

    return env.Undefined();
}

/*
 * Retrieve Dos Devices Worker
 */
class DosDevicesWorker : public AsyncWorker {
    public:
        DosDevicesWorker(Function& callback) : AsyncWorker(callback) {}
        ~DosDevicesWorker() {}

    private:
        vector<pair<char*, char*>> vDosDevices;

    // This code will be executed on the worker thread
    void Execute() {
        // TODO: Find the right memory allocation ? (double api call ?).
        char logical[65536];
        char physical[65536];

        QueryDosDeviceA(NULL, physical, sizeof(physical));
        for (char *pos = physical; *pos; pos+=strlen(pos)+1) {
            QueryDosDeviceA(pos, logical, sizeof(logical));
            vDosDevices.push_back(make_pair(pos, logical));
        }    
    }

    void OnOK() {
        HandleScope scope(Env());
        Object ret = Object::New(Env());
        for (size_t i = 0; i < vDosDevices.size(); ++i) {
            pair<char*, char*> device = vDosDevices.at(i);
            ret.Set(device.first, device.second);
        }

        Callback().Call({Env().Null(), ret});
    }

};

/*
 * Retrieve Dos Devices
 */
Value getDosDevices(const CallbackInfo& info) {
    Env env = info.Env();

    // Check argument length!
    if (info.Length() < 1) {
        Error::New(env, "Wrong number of argument provided!").ThrowAsJavaScriptException();
        return env.Null();
    }

    // callback should be typeof Napi::String
    if (!info[0].IsFunction()) {
        Error::New(env, "argument callback should be a Function!").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Execute worker thread!
    Function cb = info[0].As<Function>();
    (new DosDevicesWorker(cb))->Queue();

    return env.Undefined();
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
