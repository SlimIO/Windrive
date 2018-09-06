// Require Native addon
const windrive = require("./build/Release/windrive.node");

/**
 * @async
 * @function getLogicalDrives
 * @return {Promise<Windrive.LogicalDrive[]>}
 */
function getLogicalDrives() {
    return new Promise((resolve, reject) => {
        windrive.getLogicalDrives((error, logicalDrives) => {
            if (error) {
                return reject(error);
            }
            resolve(logicalDrives);
        });
    });
}

/**
 * @async
 * @function getDevicePerformance
 * @param {!String} driveName driveName
 * @return {Promise<Windrive.DevicePerformance>}
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
            resolve(performance);
        });
    });
}

/**
 * @async
 * @function getDosDevices
 * @return {Promise<Windrive.DosDevices>}
 */
function getDosDevices() {
    return new Promise((resolve, reject) => {
        windrive.getDosDevices((error, dosDevices) => {
            if (error) {
                return reject(error);
            }
            resolve(dosDevices);
        });
    });
}

module.exports = {
    getLogicalDrives,
    getDosDevices,
    getDevicePerformance
}
