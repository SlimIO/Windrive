declare namespace WinDisk {

    export interface LogicalDrive {
        name: string;
        bytesPerSect: number;
        driveType: number;
        freeClusters: number;
        totalClusters: number;
        usedClusterPourcent: number;
        freeClusterPourcent: number;
    }

    export interface DevicePerformance {
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

    export interface DosDevices {
        [name: string]: string;
    }

    export enum DriveType {
        UNKNOWN = 0,
        NO_ROOT_DIR = 1,
        REMOVABLE = 2,
        FIXED = 3,
        REMOTE = 4,
        CDROM = 5,
        RAMDISK = 6
    }

    export function getLogicalDrives(): LogicalDrive[];
    export function getDevicePerformance(deviceName: string): DevicePerformance;
    export function getDosDevices(): DosDevices;
}

export as namespace WinDisk;
export = WinDisk;
