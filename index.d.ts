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

    export interface DrivePerformance {
        idleTimeHigh: number;
        idleTimeLow: number;
        readCount: number;
        writeCount: number;
        queueDepth: number;
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
    export function getDrivePerformance(driveName: string): DrivePerformance;
}

export as namespace WinDisk;
export = WinDisk;
