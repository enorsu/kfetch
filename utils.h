#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <algorithm>
#include <vector>

namespace kfetch {

// --- String helpers ---------------------------------------------------------
inline std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

inline std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return result;
}

// --- Portable sysctlbyname --------------------------------------------------
// OpenBSD: emulate sysctlbyname using sysctlnametomib + sysctl
#if defined(__OpenBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <cstring>

inline int portable_sysctlbyname(const char* name,
                                 void* oldp, size_t* oldlenp,
                                 const void* newp, size_t newlen)
{
    int mib[CTL_MAXNAME];
    size_t miblen = CTL_MAXNAME;

    if (sysctlnametomib(name, mib, &miblen) == -1)
        return -1;

    return sysctl(mib, static_cast<u_int>(miblen), oldp, oldlenp,
                  const_cast<void*>(newp), newlen);
}

// Linux/macOS: use native sysctlbyname if available
#elif defined(__linux__) || defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>

inline int portable_sysctlbyname(const char* name,
                                 void* oldp, size_t* oldlenp,
                                 const void* newp, size_t newlen)
{
    return sysctlbyname(name, oldp, oldlenp, newp, newlen);
}

// Other platforms: stub returning -1 (not supported)
#else
inline int portable_sysctlbyname(const char*,
                                 void*, size_t*,
                                 const void*, size_t)
{
    return -1;
}
#endif

} // namespace kfetch

#endif // UTILS_H
