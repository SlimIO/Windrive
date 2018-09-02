/** @type {WinDisk} */
const windisk = require("./build/Release/windisk.node");

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

console.time("getLogicalDrives");
const logicalDrives = windisk.getLogicalDrives();
console.timeEnd("getLogicalDrives"); 

for (const disk of logicalDrives) {
    Reflect.set(
        disk,
        'driveTypeName',
        driveTypeName[disk.driveType]
    );

    const cleanDiskName = disk.name.substr(0, disk.name.length - 1);

    console.time("getPerf");
    const perf = windisk.getDevicePerformance(cleanDiskName);
    console.timeEnd("getPerf");
    console.log(perf);
}

// console.log(logicalDrives);
