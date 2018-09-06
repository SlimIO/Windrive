declare namespace Windrive {

    export interface LogicalDrive {
        name: string;
        bytesPerSect: number;
        type: string;
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

    export function getLogicalDrives(): Promise<LogicalDrive[]>;
    export function getDevicePerformance(deviceName: string): Promise<DevicePerformance>;
    export function getDosDevices(): Promise<DosDevices>;
}

export as namespace Windrive;
export = Windrive;
