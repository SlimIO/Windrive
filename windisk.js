/** @type {WinDisk} */
const windisk = require("./build/Release/windisk.node");

console.time("getLogicalDrives");
const logicalDrives = windisk.getLogicalDrives();
console.timeEnd("getLogicalDrives"); 

for (const disk of logicalDrives) {
    console.log(disk);
}
