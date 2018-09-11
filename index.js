/**
 * @namespace windrive
 * @desc Windows Drive (disk) & Devices - Node.JS low level binding
 */
const windrive = require("bindings")("windrive.node");

/**
 * @async
 * @function getLogicalDrives
 * @memberof windrive#
 * @desc Retrieves the currently available disk drives. An array of LogicalDrive is returned.
 * @return {Promise<Windrive.LogicalDrive[]>}
 *
 * @version 1.0.0
 * @example
 * const { getLogicalDrives } = require("@slimio/windrive");
 * async function main() {
 *     const logicalDrives = await getLogicalDrives();
 *     console.log(JSON.stringify(logicalDrives, null, 4));
 * }
 * main().catch(console.error);
 */
function getLogicalDrives() {
    return new Promise((resolve, reject) => {
        windrive.getLogicalDrives((error, logicalDrives) => {
            if (error) {
                return reject(error);
            }

            return resolve(logicalDrives);
        });
    });
}

/**
 * @async
 * @function getDevicePerformance
 * @memberof windrive#
 * @desc Provides disk performance information about a given device (drive). Return a DevicePerformance Object.
 * @param {!String} driveName driveName
 * @return {Promise<Windrive.DevicePerformance>}
 *
 * @throws {TypeError}
 *
 * @version 1.0.0
 * @example
 * const { getLogicalDrives, getDevicePerformance } = require("@slimio/windrive");
 * async function main() {
 *     const logicalDrives = await getLogicalDrives();
 *     for (const lDrive of logicalDrives) {
 *         const drivePerformance = getDevicePerformance(lDrive.name);
 *         console.log(drivePerformance);
 *     }
 * }
 * main().catch(console.error);
 */
function getDevicePerformance(driveName) {
    if (typeof driveName !== "string") {
        throw new TypeError("driveName should be typeof string!");
    }
    if (driveName.charAt(2) === "\\") {
        driveName = driveName.substr(0, driveName.length - 1);
    }

    return new Promise((resolve, reject) => {
        windrive.getDevicePerformance(driveName, (error, performance) => {
            if (error) {
                return reject(error);
            }

            return resolve(performance);
        });
    });
}

/**
 * @async
 * @function getDosDevices
 * @memberof windrive#
 * @desc Retrieves information about MS-DOS device names.
 * @return {Promise<Windrive.DosDevices>}
 *
 * @version 1.0.0
 * @example
 * const { getDosDevices } = require("@slimio/windrive");
 * async function main() {
 *     const devices = await getDosDevices();
 *     console.log(JSON.stringify(devices, null, 4));
 * }
 * main().catch(console.error);
 */
function getDosDevices() {
    return new Promise((resolve, reject) => {
        windrive.getDosDevices((error, dosDevices) => {
            if (error) {
                return reject(error);
            }

            return resolve(dosDevices);
        });
    });
}

/**
 * @async
 * @function getDeviceGeometry
 * @memberof windrive#
 * @desc Describes the geometry of disk devices and media. Return a DeviceGeometry Object.
 * @param {!String} driveName driveName
 * @return {Promise<Windrive.DeviceGeometry>}
 *
 * @throws {TypeError}
 *
 * @version 1.0.0
 * @example
 * const { getLogicalDrives, getDeviceGeometry } = require("@slimio/windrive");
 * async function main() {
 *     const logicalDrives = await getLogicalDrives();
 *     for (const lDrive of logicalDrives) {
 *         const driveGeometry = getDeviceGeometry(lDrive.name);
 *         console.log(driveGeometry);
 *     }
 * }
 * main().catch(console.error);
 */
function getDeviceGeometry(driveName) {
    if (typeof driveName !== "string") {
        throw new TypeError("driveName should be typeof string!");
    }
    if (driveName.charAt(2) === "\\") {
        driveName = driveName.substr(0, driveName.length - 1);
    }

    return new Promise((resolve, reject) => {
        windrive.getDeviceGeometry(driveName, (error, geometry) => {
            if (error) {
                return reject(error);
            }

            return resolve(geometry);
        });
    });
}

/**
 * @async
 * @function getDiskCacheInformation
 * @memberof windrive#
 * @desc Provides information about the disk cache. Return a DiskCacheInformation Object.
 * @param {!String} driveName driveName
 * @return {Promise<Windrive.DiskCacheInformation>}
 *
 * @throws {TypeError}
 */
function getDiskCacheInformation(driveName) {
    if (typeof driveName !== "string") {
        throw new TypeError("driveName should be typeof string!");
    }
    if (driveName.charAt(2) === "\\") {
        driveName = driveName.substr(0, driveName.length - 1);
    }

    return new Promise((resolve, reject) => {
        windrive.getDiskCacheInformation(driveName, (error, cacheinfo) => {
            if (error) {
                return reject(error);
            }

            return resolve(cacheinfo);
        });
    });
}

// Export all methods
module.exports = {
    getLogicalDrives,
    getDosDevices,
    getDevicePerformance,
    getDiskCacheInformation,
    getDeviceGeometry
};
