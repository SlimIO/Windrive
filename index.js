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

async function main() {
    for (const deviceName of Object.keys(dosDevices)) {
        console.log(`deviceName: ${deviceName}\n`);
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

