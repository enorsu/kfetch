#include "distros.h"
#include "utils.h"
#include "config/config.h"
#include "gpu/gpu.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <cstdlib>
#include <iomanip>
#include <chrono>

// Platform-specific includes
#ifdef __linux__
    #include <sys/sysinfo.h>
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__)
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <sys/time.h>
    #define BSD_SYSTEM
#endif

namespace kfetch {

class SystemInfo {
private:
    kfetch::Config config;
    std::string distro_name;
    std::string distro_pretty_name;
    std::string hostname;
    std::string username;
    std::string kernel;
    std::string uptime;
    std::string shell;
    std::string desktop_env;
    std::string terminal;
    std::string cpu;
    std::string gpu;
    std::string memory;
    std::string packages;
    
    std::string toLower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                      [](unsigned char c){ return std::tolower(c); });
        return result;
    }
    
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, (last - first + 1));
    }
    
    std::string readFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return "";
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    std::string executeCommand(const std::string& cmd) {
        char buffer[128];
        std::string result = "";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        return trim(result);
    }
    
    void detectDistro() {
        // Try /etc/os-release first (standard for most modern distros)
        std::ifstream os_release("/etc/os-release");
        if (os_release.is_open()) {
            std::string line;
            while (std::getline(os_release, line)) {
                if (line.find("PRETTY_NAME=") == 0) {
                    distro_pretty_name = line.substr(12);
                    distro_pretty_name.erase(std::remove(distro_pretty_name.begin(), 
                                                         distro_pretty_name.end(), '"'), 
                                            distro_pretty_name.end());
                }
                if (line.find("ID=") == 0 && line.find("ID_") != 0) {
                    distro_name = line.substr(3);
                    distro_name.erase(std::remove(distro_name.begin(), 
                                                  distro_name.end(), '"'), 
                                     distro_name.end());
                }
            }
            os_release.close();
        }
        
        // Fallback detection for specific distros
        if (distro_name.empty()) {
            if (std::ifstream("/etc/debian_version").good()) {
                distro_name = "debian";
                distro_pretty_name = "Debian GNU/Linux";
            } else if (std::ifstream("/etc/redhat-release").good()) {
                std::string content = readFile("/etc/redhat-release");
                if (content.find("Fedora") != std::string::npos) {
                    distro_name = "fedora";
                } else if (content.find("CentOS") != std::string::npos) {
                    distro_name = "centos";
                } else if (content.find("Red Hat") != std::string::npos) {
                    distro_name = "rhel";
                }
                distro_pretty_name = trim(content);
            } else if (std::ifstream("/etc/arch-release").good()) {
                distro_name = "arch";
                distro_pretty_name = "Arch Linux";
            } else if (std::ifstream("/etc/gentoo-release").good()) {
                distro_name = "gentoo";
                distro_pretty_name = "Gentoo Linux";
            } else if (std::ifstream("/etc/slackware-version").good()) {
                distro_name = "slackware";
                distro_pretty_name = trim(readFile("/etc/slackware-version"));
            }
        }
        
        // BSD detection
        struct utsname uts;
        if (uname(&uts) == 0) {
            std::string sysname = toLower(std::string(uts.sysname));
            if (sysname == "freebsd") {
                distro_name = "freebsd";
                distro_pretty_name = "FreeBSD " + std::string(uts.release);
            } else if (sysname == "openbsd") {
                distro_name = "openbsd";
                distro_pretty_name = "OpenBSD " + std::string(uts.release);
            } else if (sysname == "netbsd") {
                distro_name = "netbsd";
                distro_pretty_name = "NetBSD " + std::string(uts.release);
            } else if (sysname == "dragonfly") {
                distro_name = "dragonfly";
                distro_pretty_name = "DragonFly BSD " + std::string(uts.release);
            }
        }
        
        // Handle special cases and normalize names
        if (distro_name == "linuxmint") distro_name = "mint";
        if (distro_name == "popos") distro_name = "pop_os";
        if (distro_name == "elementary") distro_name = "elementary";
        if (distro_name == "zorin") distro_name = "zorin";
        if (distro_name == "kali") distro_name = "kali";
        if (distro_name == "parrot") distro_name = "parrot";
        if (distro_name == "endeavouros") distro_name = "endeavouros";
        if (distro_name == "artixlinux") distro_name = "artix";
        if (distro_name == "rocky") distro_name = "rocky";
        if (distro_name == "almalinux") distro_name = "almalinux";
        if (distro_name == "mxlinux" || distro_name == "mx") distro_name = "mx";
        
        // Convert to lowercase for matching
        distro_name = toLower(distro_name);
        
        if (distro_pretty_name.empty()) {
            distro_pretty_name = "Unknown System";
        }
    }
    
    void getHostname() {
        char buffer[256];
        if (gethostname(buffer, sizeof(buffer)) == 0) {
            hostname = std::string(buffer);
        }
    }
    
    void getUsername() {
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            username = std::string(pw->pw_name);
        }
    }
    
    void getKernel() {
        struct utsname uts;
        if (uname(&uts) == 0) {
            kernel = std::string(uts.sysname) + " " + std::string(uts.release);
        }
    }

    void getGPU() {
    	kfetch::GPUInfo gpu_info;
	gpu = gpu_info.getFormatted();
    }
    
    void getUptime() {
#ifdef __linux__
        struct sysinfo si;
        if (sysinfo(&si) == 0) {
            long seconds = si.uptime;
            formatUptime(seconds);
        }
#elif defined(BSD_SYSTEM)
        struct timeval boottime;
        size_t size = sizeof(boottime);
        if (sysctlbyname("kern.boottime", &boottime, &size, NULL, 0) == 0) {
            time_t now;
            time(&now);
            long seconds = static_cast<long>(difftime(now, boottime.tv_sec));
            formatUptime(seconds);
        }
#else
        // Fallback: try reading /proc/uptime
        std::ifstream uptimeFile("/proc/uptime");
        if (uptimeFile.is_open()) {
            double uptimeSeconds;
            uptimeFile >> uptimeSeconds;
            formatUptime(static_cast<long>(uptimeSeconds));
            uptimeFile.close();
        }
#endif
    }
    
    void formatUptime(long seconds) {
        int days = seconds / 86400;
        int hours = (seconds % 86400) / 3600;
        int minutes = (seconds % 3600) / 60;
        
        std::stringstream ss;
        if (days > 0) {
            ss << days << " day" << (days > 1 ? "s" : "") << ", ";
        }
        if (hours > 0) {
            ss << hours << " hour" << (hours > 1 ? "s" : "") << ", ";
        }
        ss << minutes << " min" << (minutes > 1 ? "s" : "");
        uptime = ss.str();
    }
    
    void getShell() {
    const char* shell_env = std::getenv("SHELL");
    if (shell_env) {
        std::string shell_path = std::string(shell_env);
        size_t last_slash = shell_path.find_last_of("/");
        if (last_slash != std::string::npos) {
            shell = shell_path.substr(last_slash + 1);
        } else {
            shell = shell_path;
        }
    } else {
        // Fallback for FreeBSD and other systems
        struct passwd *pw = getpwuid(getuid());
        if (pw && pw->pw_shell) {
            std::string shell_path = std::string(pw->pw_shell);
            size_t last_slash = shell_path.find_last_of("/");
            if (last_slash != std::string::npos) {
                shell = shell_path.substr(last_slash + 1);
            } else {
                shell = shell_path;
            }
        } else {
            shell = "Unknown";
        }
    }
    
    // Handle special cases
    if (shell == "bash") shell = "bash";
    else if (shell == "zsh") shell = "zsh";
    else if (shell == "fish") shell = "fish";
    else if (shell == "tcsh") shell = "tcsh";
    else if (shell == "csh") shell = "csh";
    else if (shell == "ksh") shell = "ksh";
    else if (shell == "dash") shell = "dash";
    else if (shell == "sh") {
        // Try to detect actual shell for sh symlink
        std::string real_shell = executeCommand("readlink -f $(which sh) | xargs basename");
        if (!real_shell.empty() && real_shell != "sh") {
            shell = real_shell;
        }
    }
}    

    void getDesktopEnvironment() {
        const char* de = std::getenv("XDG_CURRENT_DESKTOP");
        if (de) {
            desktop_env = std::string(de);
        } else {
            de = std::getenv("DESKTOP_SESSION");
            if (de) {
                desktop_env = std::string(de);
            } else {
                desktop_env = "None (TTY)";
            }
        }
    }
    
    void getTerminal() {
        const char* term = std::getenv("TERM_PROGRAM");
        if (term) {
            terminal = std::string(term);
        } else {
            // Try to detect from parent process
            std::string ppid = executeCommand("ps -o ppid= -p $$");
            if (!ppid.empty()) {
                std::string parent = executeCommand("ps -o comm= -p " + ppid);
                if (!parent.empty()) {
                    terminal = parent;
                }
            }
            
            if (terminal.empty()) {
                term = std::getenv("TERM");
                if (term) {
                    terminal = std::string(term);
                }
            }
        }
    }
    
    void getCPU() {
#ifdef __linux__
        std::ifstream cpuinfo("/proc/cpuinfo");
        if (cpuinfo.is_open()) {
            std::string line;
            while (std::getline(cpuinfo, line)) {
                if (line.find("model name") != std::string::npos) {
                    size_t colon = line.find(":");
                    if (colon != std::string::npos) {
                        cpu = trim(line.substr(colon + 1));
                        // Simplify CPU name
                        size_t at = cpu.find("@");
                        if (at != std::string::npos) {
                            cpu = trim(cpu.substr(0, at));
                        }
                        break;
                    }
                }
            }
            cpuinfo.close();
        }
#elif defined(BSD_SYSTEM)
        char cpu_model[256];
        size_t size = sizeof(cpu_model);
        if (sysctlbyname("hw.model", cpu_model, &size, NULL, 0) == 0) {
            cpu = std::string(cpu_model);
            // Simplify CPU name
            size_t at = cpu.find("@");
            if (at != std::string::npos) {
                cpu = trim(cpu.substr(0, at));
            }
        }
#endif
        
        if (cpu.empty()) {
            cpu = "Unknown CPU";
        }
    }

    void getMemory() {
#ifdef __linux__
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        uint64_t total_mb = (uint64_t)si.totalram * si.mem_unit / (1024ULL * 1024ULL);
        uint64_t used_mb = ((uint64_t)si.totalram - si.freeram - si.bufferram) * si.mem_unit / (1024ULL * 1024ULL);

        std::stringstream ss;
        ss << used_mb << " MB / " << total_mb << " MB";
        memory = ss.str();
        return;
    }
#elif defined(BSD_SYSTEM)
    uint64_t total_mem;
    size_t size = sizeof(total_mem);

    if (sysctlbyname("hw.physmem", &total_mem, &size, NULL, 0) != 0) {
        memory = "Unknown";
        return;
    }

    // 64-bit page info
    uint64_t free_pages = 0, inactive_pages = 0, cache_pages = 0, pagesize = 4096;

    size = sizeof(pagesize);
    sysctlbyname("hw.pagesize", &pagesize, &size, NULL, 0);

    size = sizeof(free_pages);
    sysctlbyname("vm.stats.vm.v_free_count", &free_pages, &size, NULL, 0);

    size = sizeof(inactive_pages);
    sysctlbyname("vm.stats.vm.v_inactive_count", &inactive_pages, &size, NULL, 0);

    size = sizeof(cache_pages);
    sysctlbyname("vm.stats.vm.v_cache_count", &cache_pages, &size, NULL, 0);

    uint64_t total_mb = total_mem / (1024ULL * 1024ULL);
    uint64_t available_mb = (free_pages + inactive_pages + cache_pages) * pagesize / (1024ULL * 1024ULL);
    uint64_t used_mb = (total_mb > available_mb) ? total_mb - available_mb : 0;

    std::stringstream ss;
    ss << used_mb << " MB / " << total_mb << " MB";
    memory = ss.str();
    return;
#else
    // Fallback for other systems (Linux-style /proc/meminfo)
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        uint64_t total_kb = 0, free_kb = 0, buffers_kb = 0, cached_kb = 0;

        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") == 0)
                total_kb = std::stoull(line.substr(9));
            else if (line.find("MemFree:") == 0)
                free_kb = std::stoull(line.substr(8));
            else if (line.find("Buffers:") == 0)
                buffers_kb = std::stoull(line.substr(8));
            else if (line.find("Cached:") == 0)
                cached_kb = std::stoull(line.substr(7));
        }
        meminfo.close();

        if (total_kb > 0) {
            uint64_t used_kb = total_kb - free_kb - buffers_kb - cached_kb;
            std::stringstream ss;
            ss << (used_kb / 1024ULL) << " MB / " << (total_kb / 1024ULL) << " MB";
            memory = ss.str();
            return;
        }
    }
#endif

    memory = "Unknown";
}

    void getPackages() {
    int count = 0;
    std::string manager;
    
    // FreeBSD pkg detection first
    if (system("which pkg > /dev/null 2>&1") == 0) {
        std::string result = executeCommand("pkg info -a 2>/dev/null | wc -l");
        if (!result.empty()) {
            count = std::stoi(result);
            manager = "pkg";
        }
    }
    // Try different package managers
    else if (std::ifstream("/var/lib/dpkg/status").good()) {
        std::string cmd = "dpkg-query -f '${binary:Package}\\n' -W 2>/dev/null | wc -l";
        std::string result = executeCommand(cmd);
        if (!result.empty()) {
            count = std::stoi(result);
            manager = "dpkg";
        }
    } else if (std::ifstream("/var/lib/rpm").good()) {
        std::string result = executeCommand("rpm -qa 2>/dev/null | wc -l");
        if (!result.empty()) {
            count = std::stoi(result);
            manager = "rpm";
        }
    } else if (system("which pacman > /dev/null 2>&1") == 0) {
        std::string result = executeCommand("pacman -Q 2>/dev/null | wc -l");
        if (!result.empty()) {
            count = std::stoi(result);
            manager = "pacman";
        }
    } else if (system("which emerge > /dev/null 2>&1") == 0) {
        std::string result = executeCommand("qlist -I 2>/dev/null | wc -l");
        if (!result.empty()) {
            count = std::stoi(result);
            manager = "portage";
        }
    } else if (system("which xbps-query > /dev/null 2>&1") == 0) {
        std::string result = executeCommand("xbps-query -l 2>/dev/null | wc -l");
        if (!result.empty()) {
            count = std::stoi(result);
            manager = "xbps";
        }
    } else if (system("which apk > /dev/null 2>&1") == 0) {
        std::string result = executeCommand("apk list --installed 2>/dev/null | wc -l");
        if (!result.empty()) {
            count = std::stoi(result);
            manager = "apk";
        }
    }
    
    if (count > 0) {
        std::stringstream ss;
        ss << count << " (" << manager << ")";
        packages = ss.str();
    } else {
        packages = "Unknown";
    }
}    
    
public:
    SystemInfo(int argc = 0, char* argv[] = nullptr) {
	// Load config
	std::string home = std::getenv("HOME");
	config.loadFromFile(home + "/.config/kfetch.conf");

	// Pares command line arguments
	if (argc > 0 && argv != nullptr) {
	    config.parseArgs(argc, argv);
	}

	//Get system info
        detectDistro();
        getHostname();
        getUsername();
        getKernel();
        getUptime();
        getShell();
        getDesktopEnvironment();
        getTerminal();
        getCPU();
	getGPU();
        getMemory();
        getPackages();
    
}

        void display() {
    DistroArt art = getDistroArt(distro_name);

    // Use custom art color if specified
    if (!config.custom_art_color.empty()) {
        art.color_code = config.custom_art_color;
    }

    // Info lines as pairs: {label, value}
    std::vector<std::pair<std::string, std::string>> info_pairs;

    if (config.show_username && config.show_hostname) {
        info_pairs.emplace_back("", username + "@" + hostname);
        info_pairs.emplace_back("", std::string(username.length() + hostname.length() + 1, '-'));
    } else if (config.show_username) {
        info_pairs.emplace_back("", username);
        info_pairs.emplace_back("", std::string(username.length(), '-'));
    } else if (config.show_hostname) {
        info_pairs.emplace_back("", hostname);
        info_pairs.emplace_back("", std::string(hostname.length(), '-'));
    }

    if (config.show_os) info_pairs.emplace_back("OS: ", distro_pretty_name);
    if (config.show_kernel) info_pairs.emplace_back("Kernel: ", kernel);
    if (config.show_uptime) info_pairs.emplace_back("Uptime: ", uptime);
    if (config.show_packages) info_pairs.emplace_back("Packages: ", packages);
    if (config.show_shell) info_pairs.emplace_back("Shell: ", shell);
    if (config.show_de) info_pairs.emplace_back("DE/WM: ", desktop_env);
    if (config.show_terminal) info_pairs.emplace_back("Terminal: ", terminal);
    if (config.show_cpu) info_pairs.emplace_back("CPU: ", cpu);
    if (config.show_memory) info_pairs.emplace_back("Memory: ", memory);
    info_pairs.emplace_back("GPU: ", gpu);

    // Color blocks if enabled
    if (config.show_colors) {
        info_pairs.emplace_back("", "");
        std::string color_blocks;
        for (int i = 0; i < 8; i++) {
            color_blocks += "\033[4" + std::to_string(i) + "m   ";
        }
        color_blocks += RESET_COLOR;
        info_pairs.emplace_back("", color_blocks);
    }

    size_t art_lines = config.show_art ? art.art.size() : 0;
    size_t max_lines = std::max(art_lines, info_pairs.size());

    for (size_t i = 0; i < max_lines; i++) {
        // Print ASCII art line
        if (config.show_art) {
            if (i < art.art.size()) {
                std::cout << art.color_code << art.art[i] << RESET_COLOR;
            } else {
                std::cout << std::string(art.art[0].length(), ' ');
            }
            std::cout << "  "; // spacing
        }

        // Print info line with distro-colored label
        if (i < info_pairs.size()) {
            const auto& [label, value] = info_pairs[i];

            if (!label.empty()) {
                std::cout << art.color_code << label << RESET_COLOR;
            }

            // Value can use custom text color or default
            if (!config.custom_text_color.empty()) {
                std::cout << config.custom_text_color;
            }

            std::cout << value;

            if (!config.custom_text_color.empty()) {
                std::cout << RESET_COLOR;
            }
        }

        std::cout << std::endl;
    }
  }
};

} // namespace kfetch

int main(int argc, char* argv[]) {
    std::cout << "\n";

    kfetch::SystemInfo sysinfo(argc, argv);
    sysinfo.display();
    return 0;
}
