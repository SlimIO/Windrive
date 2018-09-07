# Windrive

SlimIO Windrive is a NodeJS binding which expose low-level Microsoft APIs on Logical Drive, Disk and Devices.

This binding expose the following methods/struct:

- [GetLogicalDrives](https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getlogicaldrives)
- [GetDiskFreeSpace](https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getdiskfreespacea)
- [GetDriveType](https://docs.microsoft.com/en-us/windows/desktop/api/fileapi/nf-fileapi-getdrivetypea)
- [QueryDosDevice](https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-querydosdevicea)
- [DISK_PERFORMANCE](https://docs.microsoft.com/en-us/windows/desktop/api/winioctl/ns-winioctl-_disk_performance)
- [DISK_GEOMETRY](https://docs.microsoft.com/en-us/windows/desktop/api/winioctl/ns-winioctl-_disk_geometry)
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

Retrieve all logical drives and get each disk performance !

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

### getLogicalDrives(): Promise<LogicalDrive[]>
Retrieves the currently available disk drives. An array of LogicalDrive is returned.

```ts
interface LogicalDrive {
    name: string;
    bytesPerSect: number;
    type: string;
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

### getDosDevices(): Promise<DosDevices>
Retrieves information about MS-DOS device names. Return an key -> value Object where the key is the device name and value the path to the device.

```ts
interface DosDevices {
    [name: string]: string;
}
```

### getDevicePerformance(deviceName: string): Promise<DevicePerformance>
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

### getDiskCacheInformation(deviceName: string): Promise<DiskCacheInformation>
Provides information about the disk cache. Return a DiskCacheInformation Object.

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

### getDeviceGeometry(deviceName: string): Promise<DeviceGeometry>
Describes the geometry of disk devices and media. Return a DeviceGeometry Object.

```ts
interface DeviceGeometry {
    mediaType: number;
    cylinders: number;
    bytesPerSector: number;
    sectorsPerTrack: number;
    tracksPerCylinder: number;
}
```

## How to build the project

Before building the project, be sure to get the following installed:

- [Windows build tools](https://www.npmjs.com/package/windows-build-tools)

And execute these commands:

```bash
$ npm install
$ node-gyp configure
$ node-gyp build
```

## Roadmap 1.1.0

- Add complete tests & coverage support.
- Add more comments to improve support.
- Add CPPLint to lint cpp files
