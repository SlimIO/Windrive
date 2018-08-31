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
}

export as namespace WinDisk;
export = WinDisk;
