#include <windows.h>
#include <string>

namespace Slimio {
    std::string guidToString(GUID guid) {
        char guid_cstr[39];
        snprintf(guid_cstr, sizeof(guid_cstr),
                "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                guid.Data1, guid.Data2, guid.Data3,
                guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

        return std::string(guid_cstr);
    }

    std::string getLastErrorMessage() {
        char err[256];
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, NULL);
        return std::string(err);
    }
}
