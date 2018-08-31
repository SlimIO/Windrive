#include "Disk.h"

Disk::Disk() {}

vector<diskInfo> Disk::getAllDiskInformation() {
    vector<diskInfo> ret;

    TCHAR szBuffer[LogicalDriverLength];
    DWORD dwResult = GetLogicalDriveStrings(LogicalDriverLength, szBuffer);

    if (dwResult > 0 && dwResult <= LogicalDriverLength) {
        TCHAR *lpRootPathName = szBuffer;
        while (*lpRootPathName) {
            DWORD dwSectPerClust, dwBytesPerSect, dwFreeClusters, dwTotalClusters;
            bool fResult = GetDiskFreeSpace(
                lpRootPathName,
                &dwSectPerClust,
                &dwBytesPerSect,
                &dwFreeClusters,
                &dwTotalClusters
            );
            UINT driveType = GetDriveType(lpRootPathName);

            if(fResult && dwBytesPerSect != 0 ){
                diskInfo handle;
                CString driveName = CString(lpRootPathName);
                handle.Name = (char*) driveName.GetBuffer(driveName.GetLength());
                driveName.ReleaseBuffer();
                handle.DriveType = (double) driveType;
                handle.FreeClusters = (double) dwFreeClusters;
                handle.TotalClusters = (double) dwTotalClusters;
                handle.FreeClusterPourcent = (handle.FreeClusters / handle.TotalClusters) * 100;
                handle.UsedClusterPourcent = 100 - handle.FreeClusterPourcent;

                ret.push_back(handle);
            }
    
            lpRootPathName = &lpRootPathName[_tcslen(lpRootPathName) + 1];
        }
    }

    return ret;
}
