const windrive = require("../index");

async function main() {
    console.time("getLogicalDrives");
    const drives = await windrive.getLogicalDrives();
    console.timeEnd("getLogicalDrives");
    console.log(`${drives.length} logicial drives retrieved!`);

    console.time("getDosDevices");
    const devices = await windrive.getDosDevices();
    console.timeEnd("getDosDevices");
    console.log(`${Object.keys(devices).length} Dos Devices retrieved!`);

    for (const drive of drives) {
        console.log(`driveName: ${drive.name}`);
        console.time("getDevicePerformance");
        const perf = await windrive.getDevicePerformance(drive.name);
        console.timeEnd("getDevicePerformance");
        console.log(perf);
    }
}
main().catch(console.error);
