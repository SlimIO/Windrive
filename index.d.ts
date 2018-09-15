declare namespace Windrive {

    export type LogicalDriveType = "UNKNOWN" | "NO_ROOT_DIR" | "REMOVABLE" | "FIXED" | "REMOTE" | "CDROM" | "RAMDISK";

    export interface LogicalDrive {
        name: string;
        type: LogicalDriveType;
        bytesPerSect?: number;
        freeClusters?: number;
        totalClusters?: number;
        usedClusterPourcent?: number;
        freeClusterPourcent?: number;
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

    export interface DeviceGeometry {
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

    export interface DiskCacheInformation {
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

    export interface DosDevices {
        [name: string]: string;
    }

    export function getLogicalDrives(): Promise<LogicalDrive[]>;
    export function getDiskCacheInformation(deviceName: string): Promise<DiskCacheInformation>;
    export function getDevicePerformance(deviceName: string): Promise<DevicePerformance>;
    export function getDeviceGeometry(deviceName: string): Promise<DeviceGeometry>;
    export function getDosDevices(): Promise<DosDevices>;
}

export as namespace Windrive;
export = Windrive;
