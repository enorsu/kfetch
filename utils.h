#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstring>

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

inline std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(trim(token));
    }
    
    return tokens;
}

inline std::string join(const std::vector<std::string>& vec, const std::string& delimiter) {
    if (vec.empty()) return "";
    
    std::string result = vec[0];
    for (size_t i = 1; i < vec.size(); ++i) {
        result += delimiter + vec[i];
    }
    
    return result;
}

inline bool startsWith(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

inline bool endsWith(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && 
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline std::string replace(const std::string& str, const std::string& from, const std::string& to) {
    std::string result = str;
    size_t pos = 0;
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    return result;
}

// --- File/IO helpers --------------------------------------------------------
inline std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return trim(content);
}

inline std::string readFirstLine(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    
    std::string line;
    std::getline(file, line);
    return trim(line);
}

inline bool fileExists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

// --- System helpers ---------------------------------------------------------
inline std::string executeCommand(const std::string& command) {
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "";
    
    std::string result;
    char buffer[1024];
    
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    
    pclose(pipe);
    return trim(result);
}

// --- Number formatting helpers ----------------------------------------------
inline std::string formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    const int numUnits = sizeof(units) / sizeof(units[0]);
    
    double size = static_cast<double>(bytes);
    int unitIndex = 0;
    
    while (size >= 1024 && unitIndex < numUnits - 1) {
        size /= 1024;
        unitIndex++;
    }
    
    std::ostringstream oss;
    if (unitIndex == 0) {
        oss << static_cast<uint64_t>(size) << " " << units[unitIndex];
    } else {
        oss.precision(1);
        oss << std::fixed << size << " " << units[unitIndex];
    }
    
    return oss.str();
}

inline std::string formatUptime(uint64_t seconds) {
    uint64_t days = seconds / 86400;
    seconds %= 86400;
    uint64_t hours = seconds / 3600;
    seconds %= 3600;
    uint64_t minutes = seconds / 60;
    seconds %= 60;
    
    std::ostringstream oss;
    if (days > 0) {
        oss << days << "d ";
    }
    if (hours > 0 || days > 0) {
        oss << hours << "h ";
    }
    if (minutes > 0 || hours > 0 || days > 0) {
        oss << minutes << "m ";
    }
    oss << seconds << "s";
    
    return oss.str();
}

// --- Portable sysctlbyname --------------------------------------------------
#if defined(__OpenBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>

inline int portable_sysctlbyname(const char* name,
                                 void* oldp, size_t* oldlenp,
                                 const void* newp, size_t newlen)
{
    // Manual mapping for OpenBSD (no sysctlnametomib available)
    if (strcmp(name, "kern.ostype") == 0) {
        int mib[] = {CTL_KERN, KERN_OSTYPE};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "kern.osrelease") == 0) {
        int mib[] = {CTL_KERN, KERN_OSRELEASE};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "kern.version") == 0) {
        int mib[] = {CTL_KERN, KERN_VERSION};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "kern.hostname") == 0) {
        int mib[] = {CTL_KERN, KERN_HOSTNAME};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "kern.boottime") == 0) {
        int mib[] = {CTL_KERN, KERN_BOOTTIME};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.model") == 0) {
        int mib[] = {CTL_HW, HW_MODEL};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.ncpu") == 0) {
        int mib[] = {CTL_HW, HW_NCPU};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.ncpufound") == 0) {
        int mib[] = {CTL_HW, HW_NCPUFOUND};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.physmem") == 0) {
        int mib[] = {CTL_HW, HW_PHYSMEM};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.physmem64") == 0) {
        int mib[] = {CTL_HW, HW_PHYSMEM64};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.usermem") == 0) {
        int mib[] = {CTL_HW, HW_USERMEM};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.usermem64") == 0) {
        int mib[] = {CTL_HW, HW_USERMEM64};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.machine") == 0) {
        int mib[] = {CTL_HW, HW_MACHINE};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.machine_arch") == 0) {
        int mib[] = {CTL_HW, HW_MACHINE_ARCH};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.vendor") == 0) {
        int mib[] = {CTL_HW, HW_VENDOR};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.product") == 0) {
        int mib[] = {CTL_HW, HW_PRODUCT};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.version") == 0) {
        int mib[] = {CTL_HW, HW_VERSION};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.serialno") == 0) {
        int mib[] = {CTL_HW, HW_SERIALNO};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.uuid") == 0) {
        int mib[] = {CTL_HW, HW_UUID};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.diskcount") == 0) {
        int mib[] = {CTL_HW, HW_DISKCOUNT};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    if (strcmp(name, "hw.sensors") == 0) {
        int mib[] = {CTL_HW, HW_SENSORS};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    
    // VM subsystem
    if (strcmp(name, "vm.loadavg") == 0) {
        int mib[] = {CTL_VM, VM_LOADAVG};
        return sysctl(mib, 2, oldp, oldlenp, const_cast<void*>(newp), newlen);
    }
    
    // Unknown sysctl name
    return -1;
}

#elif defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>

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

#elif defined(__APPLE__)
#include <sys/types.h>
#include <sys/sysctl.h>

inline int portable_sysctlbyname(const char* name,
                                 void* oldp, size_t* oldlenp,
                                 const void* newp, size_t newlen)
{
    return sysctlbyname(name, oldp, oldlenp, const_cast<void*>(newp), newlen);
}

#else
// Linux or unsupported platforms: stub returning -1
inline int portable_sysctlbyname(const char*,
                                 void*, size_t*,
                                 const void*, size_t)
{
    return -1;
}
#endif

// --- Convenience wrappers for sysctl ---------------------------------------
inline std::string getSysctlString(const std::string& name) {
    size_t len = 0;
    if (portable_sysctlbyname(name.c_str(), nullptr, &len, nullptr, 0) == -1) {
        return "";
    }
    
    std::string result(len, '\0');
    if (portable_sysctlbyname(name.c_str(), &result[0], &len, nullptr, 0) == -1) {
        return "";
    }
    
    // Remove trailing null bytes
    result.resize(strlen(result.c_str()));
    return trim(result);
}

template<typename T>
inline T getSysctlValue(const std::string& name, T defaultValue = T{}) {
    T value;
    size_t len = sizeof(T);
    if (portable_sysctlbyname(name.c_str(), &value, &len, nullptr, 0) == -1) {
        return defaultValue;
    }
    return value;
}

// --- Color helpers ----------------------------------------------------------
namespace color {
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string DIM = "\033[2m";
    
    // Regular colors
    const std::string BLACK = "\033[30m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";
    
    // Bright colors
    const std::string BRIGHT_BLACK = "\033[90m";
    const std::string BRIGHT_RED = "\033[91m";
    const std::string BRIGHT_GREEN = "\033[92m";
    const std::string BRIGHT_YELLOW = "\033[93m";
    const std::string BRIGHT_BLUE = "\033[94m";
    const std::string BRIGHT_MAGENTA = "\033[95m";
    const std::string BRIGHT_CYAN = "\033[96m";
    const std::string BRIGHT_WHITE = "\033[97m";
}

} // namespace kfetch

#endif // UTILS_H
