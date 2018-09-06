// Require third-party module
const Benchmark = require("benchmark");

// Require internal module
const windrive = require("../index");

async function main() {
    // Create a new suite!
    const suite = new Benchmark.Suite;

    // add tests
    suite
    .add('getLogicalDrives', async function() {
        await windrive.getLogicalDrives();
    })
    .add('getDosDevices', async function() {
        await windrive.getDosDevices();
    })
    .on('cycle', function(event) {
        console.log(String(event.target));
    })
    .on("complete", () => {
        console.log("benchmark done!");
    })
    .run({ async: true });

}
main().catch(console.error);
