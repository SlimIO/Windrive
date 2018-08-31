#include <iostream>
#include <string>
#include "Disk.h"
#include "JavaScriptObject.h"
#include "node_api.h"
#include "assert.h"
using namespace std;

#define DECLARE_NAPI_METHOD(name, func)                          \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

napi_value getLogicalDrives(napi_env env, napi_callback_info info) {
    napi_status status;
    Disk DiskAdapter;

    // Create JavaScript Array
    napi_value JSInterfaceArray;
    status = napi_create_array(env, &JSInterfaceArray);
    assert(status == napi_ok);

    // Retrieve interfaces
    vector<diskInfo> drives = DiskAdapter.getAllDiskInformation();
    for (int i = 0; i < drives.size(); i++) {
        diskInfo drive = drives[i];
        JavaScriptObject JSInterfaceObject(env);

        /** Setup Properties */
        JSInterfaceObject.addString("name", drive.Name);
        JSInterfaceObject.addDouble("driveType", drive.DriveType);
        JSInterfaceObject.addDouble("freeClusters", drive.FreeClusters);
        JSInterfaceObject.addDouble("totalClusters", drive.TotalClusters);
        JSInterfaceObject.addDouble("usedClusterPourcent", drive.UsedClusterPourcent);
        JSInterfaceObject.addDouble("freeClusterPourcent", drive.FreeClusterPourcent);

        /** Create array entry **/
        napi_value index;
        status = napi_create_int32(env, i, &index);
        assert(status == napi_ok);

        status = napi_set_property(env, JSInterfaceArray, index, JSInterfaceObject.getSelf());
        assert(status == napi_ok);
    };

    return JSInterfaceArray;
}

napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_METHOD("getLogicalDrives", getLogicalDrives)
    };
    napi_status status = napi_define_properties(env, exports, sizeof(desc) / sizeof(*desc), desc);
    assert(status == napi_ok);

    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
