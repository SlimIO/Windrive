// Require Third-party dependencies
const test = require("ava");
const is = require("@slimio/is");

// Require package
const windrive = require("../index");

// CONSTANTS
const TYPE_ERROR = "driveName should be typeof string!";

// Test method getLogicalDrives
test("getLogicalDrives()", async function getLogicalDrives(assert) {
    assert.is(Reflect.has(windrive, "getLogicalDrives"), true);

    const logicalDrives = await windrive.getLogicalDrives();
    assert.is(is.array(logicalDrives), true);

    for (const drive of logicalDrives) {
        assert.is(is.plainObject(drive), true);
        assert.is(is.string(drive.name), true);
        assert.is(is.string(drive.type), true);
        if (drive.type !== "CDROM") {
            assert.is(is.number(drive.bytesPerSect), true);
            assert.is(is.number(drive.freeClusters), true);
            assert.is(is.number(drive.totalClusters), true);
            assert.is(is.number(drive.usedClusterPourcent), true);
            assert.is(is.number(drive.freeClusterPourcent), true);
        }
    }
});

// Test method getDosDevices
test("getDosDevices()", async function getDosDevices(assert) {
    assert.is(Reflect.has(windrive, "getDosDevices"), true);

    const dosDevices = await windrive.getDosDevices();
    assert.is(is.plainObject(dosDevices), true);
    for (const key of Object.keys(dosDevices)) {
        assert.is(is.string(key), true);
    }
    for (const value of Object.values(dosDevices)) {
        assert.is(is.string(value), true);
    }
});

// Test method getDevicePerformance
test("getDevicePerformance()", async function getDevicePerformance(assert) {
    assert.is(Reflect.has(windrive, "getLogicalDrives"), true);
    assert.is(Reflect.has(windrive, "getDevicePerformance"), true);

    // Method should throw TypeError on non-string arg
    const error = assert.throws(() => {
        windrive.getDevicePerformance(5);
    }, TypeError);
    assert.is(error.message, TYPE_ERROR);

    const logicalDrives = await windrive.getLogicalDrives();
    assert.is(is.array(logicalDrives), true);

    for (const drive of logicalDrives) {
        assert.is(is.plainObject(drive), true);

        const perfPayload = await windrive.getDevicePerformance(drive.name);
        assert.is(is.plainObject(perfPayload), true);
        assert.is(is.number(perfPayload.bytesRead), true);
        assert.is(is.number(perfPayload.bytesWritten), true);
        assert.is(is.number(perfPayload.readTime), true);
        assert.is(is.number(perfPayload.writeTime), true);
        assert.is(is.number(perfPayload.idleTime), true);
        assert.is(is.number(perfPayload.readCount), true);
        assert.is(is.number(perfPayload.writeCount), true);
        assert.is(is.number(perfPayload.queueDepth), true);
        assert.is(is.number(perfPayload.splitCount), true);
        assert.is(is.number(perfPayload.queryTime), true);
        assert.is(is.number(perfPayload.storageDeviceNumber), true);
        assert.is(is.string(perfPayload.storageManagerName), true);
    }
});

// Test method getDeviceGeometry
test("getDeviceGeometry()", async function getDevicePerformance(assert) {
    assert.is(Reflect.has(windrive, "getLogicalDrives"), true);
    assert.is(Reflect.has(windrive, "getDeviceGeometry"), true);

    // Method should throw TypeError on non-string arg
    const error = assert.throws(() => {
        windrive.getDeviceGeometry(5);
    }, TypeError);
    assert.is(error.message, TYPE_ERROR);

    const logicalDrives = await windrive.getLogicalDrives();
    assert.is(is.array(logicalDrives), true);

    for (const drive of logicalDrives) {
        assert.is(is.plainObject(drive), true);

        const geometryPayload = await windrive.getDeviceGeometry(drive.name);
        assert.is(is.plainObject(geometryPayload), true);
        assert.is(is.number(geometryPayload.diskSize), true);
        assert.is(is.number(geometryPayload.mediaType), true);
        assert.is(is.number(geometryPayload.cylinders), true);
        assert.is(is.number(geometryPayload.bytesPerSector), true);
        assert.is(is.number(geometryPayload.sectorsPerTrack), true);
        assert.is(is.number(geometryPayload.tracksPerCylinder), true);
        assert.is(is.string(geometryPayload.partition.diskId), true);
        assert.is(is.number(geometryPayload.partition.size), true);
        assert.is(is.string(geometryPayload.partition.style), true);
        assert.is(is.number(geometryPayload.partition.mbr.signature), true);
        assert.is(is.number(geometryPayload.partition.mbr.checksum), true);
        assert.is(is.number(geometryPayload.detection.size), true);
        assert.is(is.string(geometryPayload.detection.type), true);
    }
});

// Test method getDeviceGeometry
test("getDiskCacheInformation()", async function getDevicePerformance(assert) {
    assert.is(Reflect.has(windrive, "getLogicalDrives"), true);
    assert.is(Reflect.has(windrive, "getDiskCacheInformation"), true);

    // Method should throw TypeError on non-string arg
    const error = assert.throws(() => {
        windrive.getDiskCacheInformation(5);
    }, TypeError);
    assert.is(error.message, TYPE_ERROR);

    const logicalDrives = await windrive.getLogicalDrives();
    assert.is(is.array(logicalDrives), true);

    for (const drive of logicalDrives) {
        assert.is(is.plainObject(drive), true);

        const cacheInfo = await windrive.getDiskCacheInformation(drive.name);
        assert.is(is.plainObject(cacheInfo), true);
        assert.is(is.boolean(cacheInfo.parametersSavable), true);
        assert.is(is.boolean(cacheInfo.readCacheEnabled), true);
        assert.is(is.boolean(cacheInfo.writeCacheEnabled), true);
        assert.is(is.boolean(cacheInfo.prefetchScalar), true);
        assert.is(is.string(cacheInfo.readRetentionPriority), true);
        assert.is(is.number(cacheInfo.writeRetentionPriority), true);
        assert.is(is.number(cacheInfo.disablePrefetchTransferLength), true);
        if (cacheInfo.prefetchScalar) {
            assert.is(is.number(cacheInfo.scalarPrefetch.minimum), true);
            assert.is(is.number(cacheInfo.scalarPrefetch.maximum), true);
            assert.is(is.number(cacheInfo.scalarPrefetch.maximumBlocks), true);
        }
        else {
            assert.is(is.number(cacheInfo.blockPrefetch.minimum), true);
            assert.is(is.number(cacheInfo.blockPrefetch.maximum), true);
        }
    }
});
