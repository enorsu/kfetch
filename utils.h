#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <algorithm>
#include <vector>

#include <sys/types.h>
#include <sys/sysctl.h>
#include <cstring> // for memcpy, memset, etc.

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
#if defined(__OpenBSD__)
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

#else
inline int portable_sysctlbyname(const char* name,
                                 void* oldp, size_t* oldlenp,
                                 const void* newp, size_t newlen)
{
    return sysctlbyname(name, oldp, oldlenp, newp, newlen);
}
#endif

} // namespace kfetch

#endif // UTILS_H
