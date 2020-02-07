#include <windows.h>
#include <comdef.h>
#include <sstream>
#include <string>
#include "napi.h"
#include "slimio.h"

using namespace Slimio;

/*
 * Buffer length for logical drives names
 * TODO: Best value ? (120 is almost good for 30 disks)
 */
#define DRIVER_BUFFER_BYTE_LENGTH 120

/*
 * Asycnronous Worker to Retrieve Windows Logical Drives
 *
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getlogicaldrivestringsw
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getdrivetypea
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getdiskfreespacea
 */
class LogicalDriveWorker : public Napi::AsyncWorker {
    public:
        LogicalDriveWorker(Napi::Function& callback) : AsyncWorker(callback) {}
        ~LogicalDriveWorker() {}

    private:
        struct LogicalDrive {
            TCHAR* name;
            std::string driveType;
            DWORD bytesPerSect;
            DWORD freeClusters;
            DWORD totalClusters;
            double usedClusterPourcent;
            double freeClusterPourcent;
        };
        std::vector<LogicalDrive> vLogicalDrives;

    // This code will be executed on the worker thread
    void Execute() {
        BOOL success;
        UINT driveType;
        TCHAR szBuffer[DRIVER_BUFFER_BYTE_LENGTH];
        DWORD dwResult = GetLogicalDriveStrings(DRIVER_BUFFER_BYTE_LENGTH, szBuffer);

        // Throw error if we fail to retrieve result
        if (dwResult == 0) {
            return SetError("Unable to find any Logical Drive. GetLogicalDriveStrings() has returned 0 byte.");
        }
        else if (dwResult > DRIVER_BUFFER_BYTE_LENGTH) {
            std::stringstream error;
            error << "Insufficient buffer size (" << DRIVER_BUFFER_BYTE_LENGTH << "). GetLogicalDriveStrings() returned " << dwResult << " bytes!" << std::endl;
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
                    drive.driveType = std::string("UNKNOWN");
                    break;
                case DRIVE_NO_ROOT_DIR:
                    drive.driveType = std::string("NO_ROOT_DIR");
                    break;
                case DRIVE_REMOVABLE:
                    drive.driveType = std::string("REMOVABLE");
                    break;
                case DRIVE_FIXED:
                    drive.driveType = std::string("FIXED");
                    break;
                case DRIVE_REMOTE:
                    drive.driveType = std::string("REMOTE");
                    break;
                case DRIVE_CDROM:
                    drive.driveType = std::string("CDROM");
                    break;
                case DRIVE_RAMDISK:
                    drive.driveType = std::string("RAMDISK");
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
        Napi::HandleScope scope(Env());
        Napi::Array ret = Napi::Array::New(Env());
        for (size_t i = 0; i < vLogicalDrives.size(); ++i) {
            LogicalDrive currDrive = vLogicalDrives.at(i);
            Napi::Object currJSDrive = Napi::Object::New(Env());
            ret[i] = currJSDrive;

            currJSDrive.Set("name", currDrive.name);
            currJSDrive.Set("type", currDrive.driveType);
            if (currDrive.driveType != std::string("CDROM")) {
                currJSDrive.Set("bytesPerSect", currDrive.bytesPerSect);
                currJSDrive.Set("freeClusters", currDrive.freeClusters);
                currJSDrive.Set("totalClusters", currDrive.totalClusters);
                currJSDrive.Set("usedClusterPourcent", currDrive.usedClusterPourcent);
                currJSDrive.Set("freeClusterPourcent", currDrive.freeClusterPourcent);
            }
        }
        Callback().Call({Env().Null(), ret});
    }

};

/*
 * Retrieve Windows Logical Drives (with Drive type & Free spaces).
 */
Napi::Value getLogicalDrives(const Napi::CallbackInfo& info) {
    const Napi::Env env = info.Env();
    Napi::Function cb;

    if (info.Length() < 1) {
        Napi::Error::New(env, "Wrong number of argument provided!").ThrowAsJavaScriptException();
        return env.Null();
    }
    if (!info[0].IsFunction()) {
        Napi::Error::New(env, "argument callback should be a Function!").ThrowAsJavaScriptException();
        return env.Null();
    }

    cb = info[0].As<Napi::Function>();
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
class DiskPerformanceWorker : public Napi::AsyncWorker {
    public:
        DiskPerformanceWorker(Napi::Function& callback, std::string driveName) : AsyncWorker(callback), driveName(driveName) {}
        ~DiskPerformanceWorker() {}

    private:
        std::string driveName;
        DISK_PERFORMANCE sDiskPerformance = { 0 };

    // This code will be executed on the worker thread
    void Execute() {
        std::stringstream ss;
        ss << "\\\\.\\" << driveName; // Concat these weird carac (they are required to work).
        std::string tDriveName = ss.str();
        LPCSTR wDriveName = tDriveName.c_str();

        // Open device handle!
        HANDLE hDevice = CreateFileA(
            wDriveName, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL
        );

        // Cannot open the drive
        if (hDevice == INVALID_HANDLE_VALUE) {
            std::stringstream error;
            error << "CreateFileA() INVALID_HANDLE_VALUE for device " << driveName << " - " << getLastErrorMessage();
            return SetError(error.str());
        }

        DWORD junk = 0;
        SecureZeroMemory(&sDiskPerformance, sizeof(DISK_PERFORMANCE));
        bool success = DeviceIoControl(
            hDevice,
            IOCTL_DISK_PERFORMANCE,
            NULL,
            0,
            &sDiskPerformance,
            sizeof(sDiskPerformance),
            &junk,
            (LPOVERLAPPED) NULL
        );
        CloseHandle(hDevice);
        if (!success) {
            std::stringstream error;
            error << "IOCTL_DISK_PERFORMANCE failed with code (" << GetLastError() << ") - " << getLastErrorMessage();
            return SetError(error.str());
        }
    }

    void OnOK() {
        Napi::HandleScope scope(Env());
        Napi::Object ret = Napi::Object::New(Env());

        // Transform WCHAR to _bstr_t (to be translated into a const char*)
        // @header: comdef.h
        _bstr_t charStorageManagerName(sDiskPerformance.StorageManagerName);

        ret.Set("bytesRead", sDiskPerformance.BytesRead.QuadPart);
        ret.Set("bytesWritten", sDiskPerformance.BytesRead.QuadPart);
        ret.Set("readTime", sDiskPerformance.ReadTime.QuadPart);
        ret.Set("writeTime", sDiskPerformance.WriteTime.QuadPart);
        ret.Set("idleTime", sDiskPerformance.IdleTime.QuadPart);
        ret.Set("readCount", sDiskPerformance.ReadCount);
        ret.Set("writeCount", sDiskPerformance.WriteCount);
        ret.Set("queueDepth", sDiskPerformance.QueueDepth);
        ret.Set("splitCount", sDiskPerformance.SplitCount);
        ret.Set("queryTime", sDiskPerformance.QueryTime.QuadPart);
        ret.Set("storageDeviceNumber", sDiskPerformance.StorageDeviceNumber);
        ret.Set("storageManagerName", (const char*) charStorageManagerName);

        Callback().Call({Env().Null(), ret});
    }

};

/*
 * Binding for retrieving drive performance
 */
Napi::Value getDevicePerformance(const Napi::CallbackInfo& info) {
    const Napi::Env env = info.Env();
    std::string driveName;
    Napi::Function cb;

    if (info.Length() < 2) {
        Napi::Error::New(env, "Wrong number of argument provided!").ThrowAsJavaScriptException();
        return env.Null();
    }
    if (!info[1].IsFunction()) {
        Napi::Error::New(env, "argument callback should be a Function!").ThrowAsJavaScriptException();
        return env.Null();
    }

    driveName = info[0].As<Napi::String>().Utf8Value();
    cb = info[1].As<Napi::Function>();
    (new DiskPerformanceWorker(cb, driveName))->Queue();

    return env.Undefined();
}

/*
 * Retrieve Dos Devices Worker
 *
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-querydosdevicea
 */
class DosDevicesWorker : public Napi::AsyncWorker {
    public:
        DosDevicesWorker(Napi::Function& callback) : AsyncWorker(callback) {}
        ~DosDevicesWorker() {}

    private:
        std::vector<std::pair<char*, char*>> vDosDevices;

    // This code will be executed on the worker thread
    void Execute() {
        char logical[65536];
        char physical[65536];
        DWORD ret;

        // Get Sizeof buffer for all devices!
        ret = QueryDosDeviceA(NULL, physical, sizeof(physical));
        if (ret == 0) {
            return SetError(getLastErrorMessage());
        }

        for (char *pos = physical; *pos; pos+=strlen(pos)+1) {
            ret = QueryDosDeviceA(pos, logical, sizeof(logical));
            if (ret == 0) {
                continue;
            }
            vDosDevices.push_back(std::make_pair(pos, logical));
        }
    }

    void OnOK() {
        Napi::HandleScope scope(Env());
        Napi::Object ret = Napi::Object::New(Env());

        for (size_t i = 0; i < vDosDevices.size(); ++i) {
            std::pair<char*, char*> device = vDosDevices.at(i);
            ret.Set(device.first, device.second);
        }

        Callback().Call({Env().Null(), ret});
    }

};

/*
 * Retrieve Dos Devices
 */
Napi::Value getDosDevices(const Napi::CallbackInfo& info) {
    const Napi::Env env = info.Env();
    Napi::Function cb;

    if (info.Length() < 1) {
        Napi::Error::New(env, "Wrong number of argument provided!").ThrowAsJavaScriptException();
        return env.Null();
    }
    if (!info[0].IsFunction()) {
        Napi::Error::New(env, "argument callback should be a Function!").ThrowAsJavaScriptException();
        return env.Null();
    }

    cb = info[0].As<Napi::Function>();
    (new DosDevicesWorker(cb))->Queue();

    return env.Undefined();
}

/*
 * Device Geometry Worker
 *
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/winioctl/ns-winioctl-_disk_geometry
 */
class DeviceGeometryWorker : public Napi::AsyncWorker {
    public:
        DeviceGeometryWorker(Napi::Function& callback, std::string driveName) : AsyncWorker(callback), driveName(driveName) {}
        ~DeviceGeometryWorker() {}

    private:
        std::string driveName;
        DISK_GEOMETRY_EX sDeviceGeometry = { 0 };
        PDISK_DETECTION_INFO diskDetect;
        PDISK_PARTITION_INFO diskPartition;

    // This code will be executed on the worker thread
    void Execute() {
        std::stringstream ss;
        ss << "\\\\.\\" << driveName; // Concat these weird carac (they are required to work).
        std::string tDriveName = ss.str();
        LPCSTR wszDrive = tDriveName.c_str();

        HANDLE hDevice = CreateFileA(
            wszDrive, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL
        );

        // cannot open the drive
        if (hDevice == INVALID_HANDLE_VALUE) {
            std::stringstream error;
            error << "CreateFileA() INVALID_HANDLE_VALUE for device " << driveName << " - " << getLastErrorMessage();
            return SetError(error.str());
        }

        DWORD junk = 0;
        SecureZeroMemory(&sDeviceGeometry, sizeof(DISK_GEOMETRY_EX));
        bool bResult = DeviceIoControl(
            hDevice,
            IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
            NULL,
            0,
            &sDeviceGeometry,
            sizeof(sDeviceGeometry),
            &junk,
            (LPOVERLAPPED) NULL
        );
        if (!bResult) {
            CloseHandle(hDevice);
            std::stringstream error;
            error << "IOCTL_DISK_GET_DRIVE_GEOMETRY_EX failed with code (" << GetLastError() << ") - " << getLastErrorMessage();
            return SetError(error.str());
        }

        // Retrieve Detection & Partition information
        SecureZeroMemory(&diskDetect, sizeof(PDISK_DETECTION_INFO));
        SecureZeroMemory(&diskPartition, sizeof(PDISK_PARTITION_INFO));
        diskDetect = DiskGeometryGetDetect(&sDeviceGeometry);
        diskPartition = DiskGeometryGetPartition(&sDeviceGeometry);
        CloseHandle(hDevice);
    }

    void OnOK() {
        Napi::HandleScope scope(Env());

        Napi::Object ret = Napi::Object::New(Env());
        ret.Set("diskSize", sDeviceGeometry.DiskSize.QuadPart);
        ret.Set("mediaType", (double) sDeviceGeometry.Geometry.MediaType);
        ret.Set("cylinders", sDeviceGeometry.Geometry.Cylinders.QuadPart);
        ret.Set("bytesPerSector", sDeviceGeometry.Geometry.BytesPerSector);
        ret.Set("sectorsPerTrack", sDeviceGeometry.Geometry.SectorsPerTrack);
        ret.Set("tracksPerCylinder", sDeviceGeometry.Geometry.TracksPerCylinder);

        // Partition
        Napi::Object partition = Napi::Object::New(Env());
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
        Napi::Object mbr = Napi::Object::New(Env());
        mbr.Set("signature", diskPartition->Mbr.Signature);
        mbr.Set("checksum", diskPartition->Mbr.CheckSum);
        partition.Set("mbr", mbr);
        ret.Set("partition", partition);

        // Detection Info
        Napi::Object detection = Napi::Object::New(Env());
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
            Napi::Object int13 = Napi::Object::New(Env());
            int13.Set("driveSelect", diskDetect->Int13.DriveSelect);
            int13.Set("maxCylinders", diskDetect->Int13.MaxCylinders);
            int13.Set("sectorsPerTrack", diskDetect->Int13.SectorsPerTrack);
            int13.Set("maxHeads", diskDetect->Int13.MaxHeads);
            int13.Set("numberDrives", diskDetect->Int13.NumberDrives);
            detection.Set("int13", int13);
        }
        else if (diskDetect->DetectionType == DetectExInt13) {
            Napi::Object ExInt13 = Napi::Object::New(Env());
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
Napi::Value getDeviceGeometry(const Napi::CallbackInfo& info) {
    const Napi::Env env = info.Env();
    std::string driveName;
    Napi::Function cb;

    if (info.Length() < 2) {
        Napi::Error::New(env, "Wrong number of argument provided!").ThrowAsJavaScriptException();
        return env.Null();
    }
    if (!info[1].IsFunction()) {
        Napi::Error::New(env, "argument callback should be a Function!").ThrowAsJavaScriptException();
        return env.Null();
    }

    driveName = info[0].As<Napi::String>().Utf8Value();
    cb = info[1].As<Napi::Function>();
    (new DeviceGeometryWorker(cb, driveName))->Queue();

    return env.Undefined();
}


/*
 * Disk Cache Worker
 *
 * @doc: https://docs.microsoft.com/en-us/windows/desktop/api/winioctl/ns-winioctl-_disk_cache_information
 */
class DiskCacheWorker : public Napi::AsyncWorker {
    public:
        DiskCacheWorker(Napi::Function& callback, std::string driveName) : AsyncWorker(callback), driveName(driveName) {}
        ~DiskCacheWorker() {}

    private:
        std::string driveName;
        DISK_CACHE_INFORMATION sDiskCacheInformation = { 0 };

    // This code will be executed on the worker thread
    void Execute() {
        std::stringstream ss;
        ss << "\\\\.\\" << driveName; // Concat these weird carac (they are required to work).
        std::string tDriveName = ss.str();
        LPCSTR wszDrive = tDriveName.c_str();

        HANDLE hDevice = CreateFileA(
            wszDrive, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL
        );

        // cannot open the drive
        if (hDevice == INVALID_HANDLE_VALUE) {
            std::stringstream error;
            error << "CreateFileA() INVALID_HANDLE_VALUE for device " << driveName << " - " << getLastErrorMessage();
            return SetError(error.str());
        }

        DWORD junk = 0;
        bool bResult = DeviceIoControl(
            hDevice,
            IOCTL_DISK_GET_CACHE_INFORMATION,
            NULL,
            0,
            &sDiskCacheInformation,
            sizeof(sDiskCacheInformation),
            &junk,
            (LPOVERLAPPED) NULL
        );
        CloseHandle(hDevice);
        if (!bResult) {
            std::stringstream error;
            error << "IOCTL_DISK_GET_CACHE_INFORMATION failed with code (" << GetLastError() << ") - " << getLastErrorMessage();
            return SetError(error.str());
        }
    }

    void OnOK() {
        Napi::HandleScope scope(Env());
        Napi::Object ret = Napi::Object::New(Env());

        ret.Set("parametersSavable", Napi::Boolean::New(Env(), sDiskCacheInformation.ParametersSavable));
        ret.Set("readCacheEnabled", Napi::Boolean::New(Env(), sDiskCacheInformation.ReadCacheEnabled));
        ret.Set("writeCacheEnabled", Napi::Boolean::New(Env(), sDiskCacheInformation.WriteCacheEnabled));
        ret.Set("prefetchScalar", Napi::Boolean::New(Env(), sDiskCacheInformation.PrefetchScalar));
        switch(sDiskCacheInformation.ReadRetentionPriority) {
            case EqualPriority:
                ret.Set("readRetentionPriority", "EqualPriority");
                break;
            case KeepPrefetchedData:
                ret.Set("readRetentionPriority", "KeepPrefetchedData");
                break;
            case KeepReadData:
                ret.Set("readRetentionPriority", "KeepReadData");
                break;
        }
        ret.Set("writeRetentionPriority", (double) sDiskCacheInformation.WriteRetentionPriority);
        ret.Set("disablePrefetchTransferLength", sDiskCacheInformation.DisablePrefetchTransferLength);
        Napi::Object Block = Napi::Object::New(Env());
        if (sDiskCacheInformation.PrefetchScalar) {
            Block.Set("minimum", sDiskCacheInformation.ScalarPrefetch.Minimum);
            Block.Set("maximum", sDiskCacheInformation.ScalarPrefetch.Maximum);
            Block.Set("maximumBlocks", sDiskCacheInformation.ScalarPrefetch.MaximumBlocks);
            ret.Set("scalarPrefetch", Block);
        }
        else {
            Block.Set("minimum", sDiskCacheInformation.BlockPrefetch.Minimum);
            Block.Set("maximum", sDiskCacheInformation.BlockPrefetch.Maximum);
            ret.Set("blockPrefetch", Block);
        }

        Callback().Call({Env().Null(), ret});
    }

};

/*
 * Retrieve Disk Cache information
 */
Napi::Value getDiskCacheInformation(const Napi::CallbackInfo& info) {
    const Napi::Env env = info.Env();
    std::string driveName;
    Napi::Function cb;

    if (info.Length() < 2) {
        Napi::Error::New(env, "Wrong number of argument provided!").ThrowAsJavaScriptException();
        return env.Null();
    }
    if (!info[1].IsFunction()) {
        Napi::Error::New(env, "argument callback should be a Function!").ThrowAsJavaScriptException();
        return env.Null();
    }

    driveName = info[0].As<Napi::String>().Utf8Value();
    cb = info[1].As<Napi::Function>();
    (new DiskCacheWorker(cb, driveName))->Queue();

    return env.Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set("getLogicalDrives", Napi::Function::New(env, getLogicalDrives));
    exports.Set("getDevicePerformance", Napi::Function::New(env, getDevicePerformance));
    exports.Set("getDeviceGeometry", Napi::Function::New(env, getDeviceGeometry));
    exports.Set("getDosDevices", Napi::Function::New(env, getDosDevices));
    exports.Set("getDiskCacheInformation", Napi::Function::New(env, getDiskCacheInformation));

    return exports;
}

NODE_API_MODULE(windrive, Init)
