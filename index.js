/** @type {Windrive} */
const windisk = require("./build/Release/windrive.node");

/** @type {String[]} */
const driveTypeName = [
    "UNKNOWN",
    "NO_ROOT_DIR",
    "REMOVABLE",
    "FIXED",
    "REMOTE",
    "CDROM",
    "RAMDISK"
];

function sleep(ms){
    return new Promise(resolve=>{
        setTimeout(resolve,ms)
    })
}

console.time("getDosDevices");
const dosDevices = windisk.getDosDevices();
console.timeEnd("getDosDevices");
// console.log(JSON.stringify(dosDevices, null, 4));

const isDisk = /^[A-Za-z]{1}:{1}$/;
const isPhysicalDrive = /^PhysicalDrive[0-9]+$/;
function isStrPhysicalDrive(driveNameStr) {
    if (isDisk.test(driveNameStr) || isPhysicalDrive.test(driveNameStr)) {
        return true;
    }
    return false;
}

const filtered = Object.keys(dosDevices).filter(isStrPhysicalDrive);
async function main() {
    for (const deviceName of filtered) {
        console.log(`\ndeviceName: ${deviceName}`);
        console.log(`path: ${dosDevices[deviceName]}`);
        const perf = windisk.getDevicePerformance(deviceName);
        console.log(perf);
        await sleep(500);
    }
}
main().catch(console.error);

// console.time("getLogicalDrives");
// const logicalDrives = windisk.getLogicalDrives();
// console.timeEnd("getLogicalDrives"); 

// for (const disk of logicalDrives) {
//     Reflect.set(
//         disk,
//         'driveTypeName',
//         driveTypeName[disk.driveType]
//     );

//     const cleanDiskName = disk.name.substr(0, disk.name.length - 1);
//     console.log(`\nDisk name: ${cleanDiskName}`);

//     console.time("getPerf");
//     const perf = windisk.getDevicePerformance(cleanDiskName);
//     console.timeEnd("getPerf");
//     console.log(JSON.stringify(perf, null, 4));
// }

