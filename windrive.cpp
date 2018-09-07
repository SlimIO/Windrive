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

struct LogicalDrive {
    TCHAR* name;
    string driveType;
    DWORD bytesPerSect;
    DWORD freeClusters;
    DWORD totalClusters;
    double usedClusterPourcent;
    double freeClusterPourcent;
};

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

struct DeviceGeometry {
    LONGLONG diskSize;
    double mediaType;
    LONGLONG cylinders;
    DWORD tracksPerCylinder;
    DWORD sectorsPerTrack;
    DWORD bytesPerSector;
};

struct DiskCacheInformation {
    bool parametersSavable;
    bool readCacheEnabled;
    bool writeCacheEnabled;
    string readRetentionPriority;
    double writeRetentionPriority;
    WORD disablePrefetchTransferLength;
    bool prefetchScalar;
    union {
        struct {
            WORD minimum;
            WORD maximum;
            WORD maximumBlocks;
        } scalarPrefetch;
        struct {
            WORD minimum;
            WORD maximum;
        } blockPrefetch;
    };
};

/*
 * Convert GUID to std::string
 */
string guidToString(GUID guid) {
	char guid_cstr[39];
	snprintf(guid_cstr, sizeof(guid_cstr),
	         "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
	         guid.Data1, guid.Data2, guid.Data3,
	         guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
	         guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

	return string(guid_cstr);
}

string getLastErrorMessage() {
    char err[256];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, NULL);
    return string(err);
}


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
        if (dwResult == 0) {
            return SetError("Unable to find any Logical Drive. GetLogicalDriveStrings() has returned 0 byte.");
        }
        else if (dwResult > DRIVER_LENGTH) {
            stringstream error;
            error << "Insufficient buffer size (" << DRIVER_LENGTH << "). GetLogicalDriveStrings() returned " << dwResult << " bytes!" << endl;
            return SetError(error.str());
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
    const Env env = info.Env();

    // Check argument length!
    if (info.Length() < 1) {
        Error::New(env, "Wrong number of argument provided!").ThrowAsJavaScriptException();
        return env.Null();
    }

    // callback should be a Napi::Function
    if (!info[0].IsFunction()) {
        Error::New(env, "argument callback should be a Function!").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Execute worker
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
 * 
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
            stringstream error;
            error << "CreateFileA() returned INVALID_HANDLE_VALUE for Device (Drive): " << wszDrive << endl;
            return SetError(error.str());
        }

        DWORD junk = 0;
        bool bResult = DeviceIoControl(
            hDevice, IOCTL_DISK_PERFORMANCE, NULL, 0, &pdg, sizeof(pdg), &junk, (LPOVERLAPPED) NULL
        );
        if (!bResult) {
            return SetError(getLastErrorMessage());
        }
        CloseHandle(hDevice);

        // Transform WCHAR to _bstr_t (to be translated into a const char*)
        // @header: comdef.h
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
    const Env env = info.Env();

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
 * 
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-querydosdevicea
 */
class DosDevicesWorker : public AsyncWorker {
    public:
        DosDevicesWorker(Function& callback) : AsyncWorker(callback) {}
        ~DosDevicesWorker() {}

    private:
        vector<pair<char*, char*>> vDosDevices;

    // This code will be executed on the worker thread
    void Execute() {
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
    const Env env = info.Env();

    // Check argument length!
    if (info.Length() < 1) {
        Error::New(env, "Wrong number of argument provided!").ThrowAsJavaScriptException();
        return env.Null();
    }

    // callback should be a Napi::Function
    if (!info[0].IsFunction()) {
        Error::New(env, "argument callback should be a Function!").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Execute worker thread!
    Function cb = info[0].As<Function>();
    (new DosDevicesWorker(cb))->Queue();

    return env.Undefined();
}

/*
 * Device Geometry Worker
 * 
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/winioctl/ns-winioctl-_disk_geometry
 */
class DeviceGeometryWorker : public AsyncWorker {
    public:
        DeviceGeometryWorker(Function& callback, string driveName) : AsyncWorker(callback), driveName(driveName) {}
        ~DeviceGeometryWorker() {}

    private:
        string driveName;
        DeviceGeometry sDeviceGeometry;
        PDISK_DETECTION_INFO diskDetect;
        PDISK_PARTITION_INFO diskPartition;

    // This code will be executed on the worker thread
    void Execute() {
        stringstream ss;
        ss << "\\\\.\\" << driveName; // Concat these weird carac (they are required to work).
        string tDriveName = ss.str();
        LPCSTR wszDrive = tDriveName.c_str();

        // Retrieve Typedef struct DISK_GEOMETRY
        DISK_GEOMETRY_EX pdg = { 0 };

        HANDLE hDevice = CreateFileA(
            wszDrive, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL
        );              

        // cannot open the drive
        if (hDevice == INVALID_HANDLE_VALUE) {
            stringstream error;
            error << "CreateFileA() returned INVALID_HANDLE_VALUE for Device (Drive): " << wszDrive << endl;
            return SetError(error.str());
        }

        DWORD junk = 0;
        bool bResult = DeviceIoControl(
            hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &pdg, sizeof(pdg), &junk, (LPOVERLAPPED) NULL
        );
        if (!bResult) {
            return SetError(getLastErrorMessage());
        }
        CloseHandle(hDevice);

        // Retrieve Detection & Partition information
        diskDetect = DiskGeometryGetDetect(&pdg);
        diskPartition = DiskGeometryGetPartition(&pdg);

        sDeviceGeometry.diskSize = pdg.DiskSize.QuadPart;
        sDeviceGeometry.mediaType = (double) pdg.Geometry.MediaType;
        sDeviceGeometry.cylinders = pdg.Geometry.Cylinders.QuadPart;
        sDeviceGeometry.bytesPerSector = pdg.Geometry.BytesPerSector;
        sDeviceGeometry.sectorsPerTrack = pdg.Geometry.SectorsPerTrack;
        sDeviceGeometry.tracksPerCylinder = pdg.Geometry.TracksPerCylinder;
    }

    void OnOK() {
        HandleScope scope(Env());

        Object ret = Object::New(Env());
        ret.Set("diskSize", sDeviceGeometry.diskSize);
        ret.Set("mediaType", sDeviceGeometry.mediaType);
        ret.Set("cylinders", sDeviceGeometry.cylinders);
        ret.Set("bytesPerSector", sDeviceGeometry.bytesPerSector);
        ret.Set("sectorsPerTrack", sDeviceGeometry.sectorsPerTrack);
        ret.Set("tracksPerCylinder", sDeviceGeometry.tracksPerCylinder);

        // Partition
        Object partition = Object::New(Env());
        partition.Set("diskId", guidToString(diskPartition->Gpt.DiskId));
        partition.Set("size", diskPartition->SizeOfPartitionInfo);
        switch(diskPartition->PartitionStyle) {
            case PARTITION_STYLE_MBR:
                partition.Set("style", "MBR");
                break;
            case PARTITION_STYLE_GPT:
                partition.Set("style", "GPT");
                break;
            case PARTITION_STYLE_RAW:
                partition.Set("style", "RAW");
                break;
        }
        Object mbr = Object::New(Env());
        mbr.Set("signature", diskPartition->Mbr.Signature);
        mbr.Set("checksum", diskPartition->Mbr.CheckSum);
        partition.Set("mbr", mbr);
        ret.Set("partition", partition);

        // Detection Info
        Object detection = Object::New(Env());
        detection.Set("size", diskDetect->SizeOfDetectInfo);

        switch(diskDetect->DetectionType) {
            case DetectExInt13:
                detection.Set("type", "ExInt13");
                break;
            case DetectInt13:
                detection.Set("type", "Int13");
                break;
            case DetectNone:
                detection.Set("type", "None");
                break;
        }
        if (diskDetect->DetectionType == DetectInt13) {
            Object int13 = Object::New(Env());
            int13.Set("driveSelect", diskDetect->Int13.DriveSelect);
            int13.Set("maxCylinders", diskDetect->Int13.MaxCylinders);
            int13.Set("sectorsPerTrack", diskDetect->Int13.SectorsPerTrack);
            int13.Set("maxHeads", diskDetect->Int13.MaxHeads);
            int13.Set("numberDrives", diskDetect->Int13.NumberDrives);
            detection.Set("int13", int13);
        }
        else if (diskDetect->DetectionType == DetectExInt13) {
            Object ExInt13 = Object::New(Env());
            ExInt13.Set("bufferSize", diskDetect->ExInt13.ExBufferSize);
            ExInt13.Set("flags", diskDetect->ExInt13.ExFlags);
            ExInt13.Set("cylinders", diskDetect->ExInt13.ExCylinders);
            ExInt13.Set("heads", diskDetect->ExInt13.ExHeads);
            ExInt13.Set("sectorsPerTrack", diskDetect->ExInt13.ExSectorsPerTrack);
            ExInt13.Set("sectorsPerDrive", diskDetect->ExInt13.ExSectorsPerDrive);
            ExInt13.Set("sectorSize", diskDetect->ExInt13.ExSectorSize);
            ExInt13.Set("reserved", diskDetect->ExInt13.ExReserved);
            detection.Set("exInt13", ExInt13);
        }
        ret.Set("detection", detection);

        Callback().Call({Env().Null(), ret});
    }

};

/*
 * Retrieve Device (Disk) Geometry
 */
Value getDeviceGeometry(const CallbackInfo& info) {
    const Env env = info.Env();

    // Check argument length!
    if (info.Length() < 2) {
        Error::New(env, "Wrong number of argument provided!").ThrowAsJavaScriptException();
        return env.Null();
    }

    // callback should be a Napi::Function
    if (!info[1].IsFunction()) {
        Error::New(env, "argument callback should be a Function!").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Execute worker thread!
    string driveName = info[0].As<String>().Utf8Value();
    Function cb = info[1].As<Function>();
    (new DeviceGeometryWorker(cb, driveName))->Queue();

    return env.Undefined();
}


/*
 * Disk Cache Worker
 * 
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/winioctl/ns-winioctl-_disk_cache_information
 */
class DiskCacheWorker : public AsyncWorker {
    public:
        DiskCacheWorker(Function& callback, string driveName) : AsyncWorker(callback), driveName(driveName) {}
        ~DiskCacheWorker() {}

    private:
        string driveName;
        DiskCacheInformation sDiskCacheInformation;

    // This code will be executed on the worker thread
    void Execute() {
        stringstream ss;
        ss << "\\\\.\\" << driveName; // Concat these weird carac (they are required to work).
        string tDriveName = ss.str();
        LPCSTR wszDrive = tDriveName.c_str();

        // Retrieve Typedef struct DISK_GEOMETRY
        DISK_CACHE_INFORMATION pdg = { 0 };

        HANDLE hDevice = CreateFileA(
            wszDrive, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL
        );              

        // cannot open the drive
        if (hDevice == INVALID_HANDLE_VALUE) {
            stringstream error;
            error << "CreateFileA() returned INVALID_HANDLE_VALUE for Device (Drive): " << wszDrive << endl;
            return SetError(error.str());
        }

        DWORD junk = 0;
        bool bResult = DeviceIoControl(
            hDevice, IOCTL_DISK_GET_CACHE_INFORMATION, NULL, 0, &pdg, sizeof(pdg), &junk, (LPOVERLAPPED) NULL
        );
        if (!bResult) {
            return SetError(getLastErrorMessage());
        }  
        CloseHandle(hDevice);

        sDiskCacheInformation.parametersSavable = pdg.ParametersSavable;
        sDiskCacheInformation.readCacheEnabled = pdg.ReadCacheEnabled;
        sDiskCacheInformation.writeCacheEnabled = pdg.WriteCacheEnabled;
        sDiskCacheInformation.prefetchScalar = pdg.PrefetchScalar;
        switch(pdg.ReadRetentionPriority) {
            case EqualPriority:
                sDiskCacheInformation.readRetentionPriority = string("EqualPriority");
                break;
            case KeepPrefetchedData:
                sDiskCacheInformation.readRetentionPriority = string("KeepPrefetchedData");
                break;
            case KeepReadData:
                sDiskCacheInformation.readRetentionPriority = string("KeepReadData");
                break;
        }
        sDiskCacheInformation.writeRetentionPriority = (double) pdg.WriteRetentionPriority;
        sDiskCacheInformation.disablePrefetchTransferLength = pdg.DisablePrefetchTransferLength;

        if (pdg.PrefetchScalar) {
            sDiskCacheInformation.scalarPrefetch.minimum = pdg.ScalarPrefetch.Minimum;
            sDiskCacheInformation.scalarPrefetch.maximum = pdg.ScalarPrefetch.Maximum;
            sDiskCacheInformation.scalarPrefetch.maximumBlocks = pdg.ScalarPrefetch.MaximumBlocks;
        }
        else {
            sDiskCacheInformation.blockPrefetch.minimum = pdg.BlockPrefetch.Minimum;
            sDiskCacheInformation.blockPrefetch.maximum = pdg.BlockPrefetch.Maximum;
        }
    }

    void OnOK() {
        HandleScope scope(Env());
    
        Object ret = Object::New(Env());
        ret.Set("parametersSavable", Boolean::New(Env(), sDiskCacheInformation.parametersSavable));
        ret.Set("readCacheEnabled", Boolean::New(Env(), sDiskCacheInformation.readCacheEnabled));
        ret.Set("writeCacheEnabled", Boolean::New(Env(), sDiskCacheInformation.writeCacheEnabled));
        ret.Set("prefetchScalar", Boolean::New(Env(), sDiskCacheInformation.prefetchScalar));
        ret.Set("readRetentionPriority", sDiskCacheInformation.readRetentionPriority);
        ret.Set("writeRetentionPriority", sDiskCacheInformation.writeRetentionPriority);
        ret.Set("disablePrefetchTransferLength", sDiskCacheInformation.disablePrefetchTransferLength);
        Object Block = Object::New(Env());
        if (sDiskCacheInformation.prefetchScalar) {
            Block.Set("minimum", sDiskCacheInformation.scalarPrefetch.minimum);
            Block.Set("maximum", sDiskCacheInformation.scalarPrefetch.maximum);
            Block.Set("maximumBlocks", sDiskCacheInformation.scalarPrefetch.maximumBlocks);
            ret.Set("scalarPrefetch", Block);
        }
        else {
            Block.Set("minimum", sDiskCacheInformation.blockPrefetch.minimum);
            Block.Set("maximum", sDiskCacheInformation.blockPrefetch.maximum);
            ret.Set("blockPrefetch", Block);
        }

        Callback().Call({Env().Null(), ret});
    }

};

/*
 * Retrieve Disk Cache information
 */
Value getDiskCacheInformation(const CallbackInfo& info) {
    const Env env = info.Env();

    // Check argument length!
    if (info.Length() < 2) {
        Error::New(env, "Wrong number of argument provided!").ThrowAsJavaScriptException();
        return env.Null();
    }

    // callback should be a Napi::Function
    if (!info[1].IsFunction()) {
        Error::New(env, "argument callback should be a Function!").ThrowAsJavaScriptException();
        return env.Null();
    }

    // Execute worker thread!
    string driveName = info[0].As<String>().Utf8Value();
    Function cb = info[1].As<Function>();
    (new DiskCacheWorker(cb, driveName))->Queue();

    return env.Undefined();
}

// Initialize Native Addon
Object Init(Env env, Object exports) {

    // Exports addon methods!
    exports.Set("getLogicalDrives", Function::New(env, getLogicalDrives));
    exports.Set("getDevicePerformance", Function::New(env, getDevicePerformance));
    exports.Set("getDeviceGeometry", Function::New(env, getDeviceGeometry));
    exports.Set("getDosDevices", Function::New(env, getDosDevices));
    exports.Set("getDiskCacheInformation", Function::New(env, getDiskCacheInformation));

    return exports;
}

// Export Addon as windrive
NODE_API_MODULE(windrive, Init)
