// Require Third-party dependencies
const test = require("ava");
const is = require("@sindresorhus/is");

// Require package
const windrive = require("../index");

// Test method getLogicalDrives
test("getLogicalDrives()", async function getLogicalDrives(t) {
    t.is(Reflect.has(windrive, "getLogicalDrives"), true);

    const logicalDrives = await windrive.getLogicalDrives();
    t.is(is.array(logicalDrives), true);

    for (const drive of logicalDrives) {
        t.is(is.plainObject(drive), true);
        t.is(is.string(drive.name), true);
        t.is(is.number(drive.bytesPerSect), true);
        t.is(is.string(drive.type), true);
        t.is(is.number(drive.freeClusters), true);
        t.is(is.number(drive.totalClusters), true);
        t.is(is.number(drive.usedClusterPourcent), true);
        t.is(is.number(drive.freeClusterPourcent), true);
    }
});

// Test method getDosDevices
test("getDosDevices()", async function getDosDevices(t) {
    t.is(Reflect.has(windrive, "getDosDevices"), true);

    const dosDevices = await windrive.getDosDevices();
    t.is(is.plainObject(dosDevices), true);
    for (const key of Object.keys(dosDevices)) {
        t.is(is.string(key), true);
    }
    for (const value of Object.values(dosDevices)) {
        t.is(is.string(value), true);
    }
});

// Test method getDevicePerformance
test("getDevicePerformance()", async function getDevicePerformance(t) {
    t.is(Reflect.has(windrive, "getLogicalDrives"), true);
    t.is(Reflect.has(windrive, "getDevicePerformance"), true);

    const logicalDrives = await windrive.getLogicalDrives();
    t.is(is.array(logicalDrives), true);

    for (const drive of logicalDrives) {
        t.is(is.plainObject(drive), true);

        const perfPayload = await windrive.getDevicePerformance(drive.name);
        t.is(is.plainObject(perfPayload), true);
        t.is(is.number(perfPayload.bytesRead), true);
        t.is(is.number(perfPayload.bytesWritten), true);
        t.is(is.number(perfPayload.readTime), true);
        t.is(is.number(perfPayload.writeTime), true);
        t.is(is.number(perfPayload.idleTime), true);
        t.is(is.number(perfPayload.readCount), true);
        t.is(is.number(perfPayload.writeCount), true);
        t.is(is.number(perfPayload.queueDepth), true);
        t.is(is.number(perfPayload.splitCount), true);
        t.is(is.number(perfPayload.queryTime), true);
        t.is(is.number(perfPayload.storageDeviceNumber), true);
        t.is(is.string(perfPayload.storageManagerName), true);
    }
});

// Test method getDeviceGeometry
test("getDeviceGeometry()", async function getDevicePerformance(t) {
    t.is(Reflect.has(windrive, "getLogicalDrives"), true);
    t.is(Reflect.has(windrive, "getDeviceGeometry"), true);

    const logicalDrives = await windrive.getLogicalDrives();
    t.is(is.array(logicalDrives), true);

    for (const drive of logicalDrives) {
        t.is(is.plainObject(drive), true);

        const geometryPayload = await windrive.getDeviceGeometry(drive.name);
        t.is(is.plainObject(geometryPayload), true);
        t.is(is.number(geometryPayload.diskSize), true);
        t.is(is.number(geometryPayload.mediaType), true);
        t.is(is.number(geometryPayload.cylinders), true);
        t.is(is.number(geometryPayload.bytesPerSector), true);
        t.is(is.number(geometryPayload.sectorsPerTrack), true);
        t.is(is.number(geometryPayload.tracksPerCylinder), true);
        t.is(is.string(geometryPayload.partition.diskId), true);
        t.is(is.number(geometryPayload.partition.size), true);
        t.is(is.string(geometryPayload.partition.style), true);
        t.is(is.number(geometryPayload.partition.mbr.signature), true);
        t.is(is.number(geometryPayload.partition.mbr.checksum), true);
        t.is(is.number(geometryPayload.detection.size), true);
        t.is(is.string(geometryPayload.detection.type), true);
    }
});

// Test method getDeviceGeometry
test("getDiskCacheInformation()", async function getDevicePerformance(t) {
    t.is(Reflect.has(windrive, "getLogicalDrives"), true);
    t.is(Reflect.has(windrive, "getDiskCacheInformation"), true);

    const logicalDrives = await windrive.getLogicalDrives();
    t.is(is.array(logicalDrives), true);

    for (const drive of logicalDrives) {
        t.is(is.plainObject(drive), true);

        const cacheInfo = await windrive.getDiskCacheInformation(drive.name);
        t.is(is.plainObject(cacheInfo), true);
        t.is(is.boolean(cacheInfo.parametersSavable), true);
        t.is(is.boolean(cacheInfo.readCacheEnabled), true);
        t.is(is.boolean(cacheInfo.writeCacheEnabled), true);
        t.is(is.boolean(cacheInfo.prefetchScalar), true);
        t.is(is.string(cacheInfo.readRetentionPriority), true);
        t.is(is.number(cacheInfo.writeRetentionPriority), true);
        t.is(is.number(cacheInfo.disablePrefetchTransferLength), true);
        if (cacheInfo.prefetchScalar) {
            t.is(is.number(cacheInfo.scalarPrefetch.minimum), true);
            t.is(is.number(cacheInfo.scalarPrefetch.maximum), true);
            t.is(is.number(cacheInfo.scalarPrefetch.maximumBlocks), true);
        }
        else {
            t.is(is.number(cacheInfo.blockPrefetch.minimum), true);
            t.is(is.number(cacheInfo.blockPrefetch.maximum), true);
        }
    }
});
