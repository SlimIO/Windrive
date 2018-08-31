#include "afxcoll.h"
#include <windows.h>
#include <string>
#include <iostream>
#include "JavaScriptObject.h"
#include "node_api.h"
#include "assert.h"
using namespace std;

#define LogicalDriverLength 150
#define DECLARE_NAPI_METHOD(name, func)                          \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

napi_value getLogicalDrives(napi_env env, napi_callback_info info) {
    napi_status status;

    // Create JavaScript Array
    napi_value JSInterfaceArray;
    status = napi_create_array(env, &JSInterfaceArray);
    assert(status == napi_ok);

    TCHAR szBuffer[LogicalDriverLength];
    DWORD dwResult = GetLogicalDriveStrings(LogicalDriverLength, szBuffer);

    if (dwResult > 0 && dwResult <= LogicalDriverLength) {
        TCHAR *lpRootPathName = szBuffer;
        unsigned int i = 0;
        while (*lpRootPathName) {
            DWORD dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;
            bool fResult = GetDiskFreeSpace(
                lpRootPathName,
                &dwSectPerClust,
                &dwBytesPerSect,
                &dwFreeClusters,
                &dwTotalClusters
            );

            if(fResult && dwBytesPerSect != 0 ){
                UINT driveType = GetDriveType(lpRootPathName);

                CString driveName = CString(lpRootPathName);
                double FreeClusters = (double) dwFreeClusters;
                double TotalClusters = (double) dwTotalClusters;
                double FreeClusterPourcent = (FreeClusters / TotalClusters) * 100;

                JavaScriptObject JSInterfaceObject(env);

                /** Setup Properties */
                JSInterfaceObject.addString("name", (char*) driveName.GetBuffer(driveName.GetLength()));
                driveName.ReleaseBuffer();
                JSInterfaceObject.addDouble("driveType", (double) driveType);
                JSInterfaceObject.addDouble("freeClusters", FreeClusters);
                JSInterfaceObject.addDouble("totalClusters", TotalClusters);
                JSInterfaceObject.addDouble("usedClusterPourcent", 100 - FreeClusterPourcent);
                JSInterfaceObject.addDouble("freeClusterPourcent", FreeClusterPourcent);

                /** Create array entry **/
                napi_value index;
                status = napi_create_int32(env, i, &index);
                assert(status == napi_ok);

                status = napi_set_property(env, JSInterfaceArray, index, JSInterfaceObject.getSelf());
                assert(status == napi_ok);
            }
    
            lpRootPathName = &lpRootPathName[_tcslen(lpRootPathName) + 1];
            i++;
        }
    }

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
