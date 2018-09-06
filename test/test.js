const windrive = require("../index");

async function main() {
    console.time("getLogicalDrives");
    const drives = await windrive.getLogicalDrives();
    console.timeEnd("getLogicalDrives");

    console.time("getDosDevices");
    const devices = await windrive.getDosDevices();
    console.timeEnd("getDosDevices");

    for (const drive of drives) {
        console.log(`driveName: ${drive.name}`);
        console.time("getDevicePerformance");
        const perf = await windrive.getDevicePerformance(drive.name);
        console.timeEnd("getDevicePerformance");
    }
}
main().catch(console.error);