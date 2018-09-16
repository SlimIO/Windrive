# Windrive

SlimIO Windrive is a NodeJS binding which expose low-level Microsoft APIs on Logical Drive, Disk and Devices.

This binding expose the following methods/struct:

- [GetLogicalDrives](https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getlogicaldrives)
- [GetDiskFreeSpace](https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getdiskfreespacea)
- [GetDriveType](https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getdrivetypea)
- [QueryDosDevice](https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-querydosdevicea)
- [DISK_PERFORMANCE](https://docs.microsoft.com/en-us/windows/desktop/api/winioctl/ns-winioctl-_disk_performance)
- [DISK_GEOMETRY_EX](https://docs.microsoft.com/en-us/windows/desktop/api/winioctl/ns-winioctl-_disk_geometry_ex)
- [DISK_CACHE_INFORMATION](https://docs.microsoft.com/en-us/windows/desktop/api/winioctl/ns-winioctl-_disk_cache_information)

> !!! All method are called asynchronously without blocking the libuv event-loop !!!

## Getting Started

This package is available in the Node Package Repository and can be easily installed with [npm](https://docs.npmjs.com/getting-started/what-is-npm) or [yarn](https://yarnpkg.com).

```bash
$ npm i @slimio/windrive
# or
$ yarn add @slimio/windrive
```

## Usage example

Get all active logical drives and retrieve disk performance for each of them!

```js
const windrive = require("@slimio/windrive");

async function main() {
    const logicalDrives = await windrive.getLogicalDrives();

    for (const drive of logicalDrives) {
        console.log(`drive name: ${drive.name}`);
        const diskPerformance = await windrive.getDevicePerformance(drive.name);
        console.log(diskPerformance);
    }
}
main().catch(console.error);
```

## API

### getLogicalDrives(): Promise< LogicalDrive[] >
Retrieves the currently available disk drives. An array of LogicalDrive is returned.

```ts
type LogicalDriveType = "UNKNOWN" | "NO_ROOT_DIR" | "REMOVABLE" | "FIXED" | "REMOTE" | "CDROM" | "RAMDISK";

interface LogicalDrive {
    name: string;
    bytesPerSect: number;
    type: LogicalDriveType;
    freeClusters: number;
    totalClusters: number;
    usedClusterPourcent: number;
    freeClusterPourcent: number;
}
```

Possible drive types are:

| type | description |
| --- | --- |
| UNKNOWN | The drive type cannot be determined. |
| NO_ROOT_DIR | The root path is invalid; for example, there is no volume mounted at the specified path. |
| REMOVABLE | The drive has removable media; for example, a floppy drive, thumb drive, or flash card reader. |
| FIXED | The drive has fixed media; for example, a hard disk drive or flash drive. |
| REMOTE | The drive is a remote (network) drive. |
| CDROM | The drive is a CD-ROM drive. |
| RAMDISK | The drive is a RAM disk. |

> CDROM Type have no FreeSpaces (only name and type are returned).

### getDosDevices(): Promise< DosDevices >
Retrieves information about MS-DOS device names. Return an key -> value Object where the key is the device name and value the path to the device.

```ts
interface DosDevices {
    [name: string]: string;
}
```
allDrivePerformance
For example, you can filter the result to retrieves Logical and **Physical** Drives information & performance:
```js
const isDisk = /^[A-Za-z]{1}:{1}$/;
const isPhysicalDrive = /^PhysicalDrive[0-9]+$/;
function isLogicalOrPhysicalDrive(driveNameStr) {
    return isDisk.test(driveNameStr) || isPhysicalDrive.test(driveNameStr) ? true : false;
}

async function main() {
    const dosDevices = await windrive.getDosDevices();
    const physicalAndLogicalDriveDevices = Object.keys(dosDevices).filter(isLogicalOrPhysicalDrive);
    const allDrivePerformance = await Promise.all(
        physicalAndLogicalDriveDevices.map(dev => windrive.getDevicePerformance(dev))
    );
    console.log(allDrivePerformance);
}
main().catch(console.error);
```

### getDevicePerformance(deviceName: string): Promise< DevicePerformance >
Provides disk performance information about a given device (drive). Return a DevicePerformance Object.

```ts
interface DevicePerformance {
    bytesRead: number;
    bytesWritten: number;
    readTime: number;
    writeTime: number;
    idleTime: number;
    readCount: number;
    writeCount: number;
    queueDepth: number;
    splitCount: number;
    queryTime: number;
    storageDeviceNumber: number;
    storageManagerName: string;
}
```

### getDiskCacheInformation(deviceName: string): Promise< DiskCacheInformation >
Provides information about the disk cache. Return a DiskCacheInformation Object.

The result of the property `prefetchScalar` define which of scalarPrefetch (**true**) or blockPrefect (**false**) should be filled/completed.

```ts
interface DiskCacheInformation {
    parametersSavable: boolean;
    readCacheEnabled: boolean;
    writeCacheEnabled: boolean;
    prefetchScalar: boolean;
    readRetentionPriority: "EqualPriority" | "KeepPrefetchedData" | "KeepReadData";
    writeRetentionPriority: number;
    disablePrefetchTransferLength: number;
    scalarPrefetch?: {
        minimum: number;
        maximum: number;
        maximumBlocks: number;
    };
    blockPrefetch?: {
        minimum: number;
        maximum: number;
    };
}
```

### getDeviceGeometry(deviceName: string): Promise< DeviceGeometry >
Describes the geometry of disk devices and media. Return a DeviceGeometry Object.

```ts
interface DeviceGeometry {
    diskSize: number;
    mediaType: number;
    cylinders: number;
    bytesPerSector: number;
    sectorsPerTrack: number;
    tracksPerCylinder: number;
    partition: {
        diskId: string;
        size: number;
        style: "MBR" | "GPT" | "RAW";
        mbr: {
            signature: number;
            checksum: number;
        }
    };
    detection: {
        size: number;
        type: "ExInt13" | "Int13" | "None";
        int13?: {
            driveSelect: number;
            maxCylinders: number;
            sectorsPerTrack: number;
            maxHeads: number;
            numberDrives: number;
        };
        exInt13?: {
            bufferSize: number;
            flags: number;
            cylinders: number;
            heads: number;
            sectorsPerTrack: number;
            sectorsPerDrive: number;
            sectorSize: number;
            reserved: number;
        };
    }
}
```

Media type enumeration can be retrieved [here](https://msdn.microsoft.com/en-us/library/windows/desktop/aa365231(v=vs.85).aspx).

## How to build the project

Before building the project, be sure to get the following npm package installed:

- Install (or upgrade to) NodeJS v10+ and npm v6+
- [Windows build tools](https://www.npmjs.com/package/windows-build-tools)

Then, just run normal npm install command:

```bash
$ npm install
```

## Available commands

All projects commands are described here:

| command | description |
| --- | --- |
| npm run prebuild | Generate addon prebuild |
| npm run doc | Generate JSDoc .HTML documentation (in the /docs root directory) |
| npm run coverage | Generate coverage of tests |
| npm run report | Generate .HTML report of tests coverage |

> the report command have to be triggered after the coverage command.
