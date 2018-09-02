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
    const perf = windisk.getDrivePerformance(disk.name);
    console.log(perf);
    console.log(perf.idleTimeHigh * 2 ^ 32 + perf.idleTimeLow);
    break;
}

console.log(logicalDrives);
