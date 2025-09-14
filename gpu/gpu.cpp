#include "gpu.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>

namespace kfetch {

// runCommand: execute a shell command and capture trimmed stdout
static std::string runCommand(const char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return {};
    char buffer[512];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe))
        result += buffer;
    pclose(pipe);
    return trim(result);
}

GPUInfo::GPUInfo() {
#ifdef __linux__
    // Try lspci if available
    if (system("command -v lspci >/dev/null 2>&1") == 0) {
        std::string out = runCommand(
            "lspci -v | grep -A 10 'VGA\\|3D' | "
            "grep -E 'VGA|3D|NVIDIA|AMD|Intel' | head -1");
        if (!out.empty()) {
            size_t colon = out.find(": ");
            gpu_name = (colon != std::string::npos)
                       ? out.substr(colon + 2) : out;
            gpu_name = trim(gpu_name);
        }
    }

    // NVIDIA query
    if (gpu_name.empty() &&
        system("command -v nvidia-smi >/dev/null 2>&1") == 0) {
        std::string out = runCommand(
            "nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null");
        if (!out.empty()) gpu_name = trim(out);
    }

#elif defined(__FreeBSD__) || defined(__DragonFly__)
    // NVIDIA query FIRST for FreeBSD
    if (system("command -v nvidia-smi >/dev/null 2>&1") == 0) {
        std::string out = runCommand(
            "nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null");
        if (!out.empty()) {
            gpu_name = trim(out);
        }
    }

    // pciconf fallback
    if (gpu_name.empty()) {
        std::string output = runCommand(
            "pciconf -lv | grep -i -A 3 -E '(vgapci|nvidia|amd|radeon|intel)'");
        if (!output.empty()) {
            std::regex vendor_re("vendor\\s*=\\s*'([^']+)'");
            std::regex device_re("device\\s*=\\s*'([^']+)'");
            std::smatch m;

            std::string vendor, device;
            if (std::regex_search(output, m, vendor_re)) vendor = m[1];
            if (std::regex_search(output, m, device_re)) device = m[1];

            if (!vendor.empty() || !device.empty())
                gpu_name = trim(vendor + " " + device);
        }
    }

    // dmesg fallback
    if (gpu_name.empty()) {
        std::string out = runCommand(
            "dmesg | grep -i -E '(nvidia|amd|radeon|intel).*graphics|vga' | head -1");
        if (!out.empty()) {
            size_t colon = out.find(":");
            gpu_name = (colon != std::string::npos)
                       ? out.substr(colon + 1) : out;
            gpu_name = trim(gpu_name);
        }
    }

    // OpenGL fallback
    if (gpu_name.empty() &&
        system("command -v glxinfo >/dev/null 2>&1") == 0) {
        gpu_name = runCommand(
            "glxinfo 2>/dev/null | grep 'OpenGL renderer string' | cut -d: -f2");
    }

#elif defined(__OpenBSD__) || defined(__NetBSD__)
    gpu_name = runCommand(
        "dmesg | grep -i 'vga\\|graphics\\|nvidia\\|amd\\|radeon' | head -1");
#endif

    if (gpu_name.empty())
        gpu_name = "Unknown GPU";
}

std::string GPUInfo::getFormatted() const {
    if (gpu_name == "Unknown GPU") return gpu_name;

    std::string simplified = gpu_name;
    size_t pos;

    auto replaceAll = [&](const std::string& from, const std::string& to) {
        while ((pos = simplified.find(from)) != std::string::npos)
            simplified.replace(pos, from.size(), to);
    };

    // Replace known vendor strings
    replaceAll("Advanced Micro Devices", "AMD");
    replaceAll("Intel Corporation", "Intel");
    replaceAll("NVIDIA Corporation", "NVIDIA");
    replaceAll("Corporation", "");
    replaceAll("Inc.", "");

    // Remove vendor/device prefixes if present
    if (simplified.rfind("vendor ", 0) == 0)
        simplified = simplified.substr(7);
    if (simplified.rfind("device ", 0) == 0)
        simplified = simplified.substr(7);

    // Remove quotes
    simplified.erase(std::remove(simplified.begin(),
                                 simplified.end(), '\''), simplified.end());
    simplified.erase(std::remove(simplified.begin(),
                                 simplified.end(), '\"'), simplified.end());

    // Remove bracketed aliases like [AMD/ATI]
    std::regex brackets("\\[[^\\]]*\\]");
    simplified = std::regex_replace(simplified, brackets, "");

    // Collapse multiple spaces
    std::regex multiSpace("\\s{2,}");
    simplified = std::regex_replace(simplified, multiSpace, " ");

    // Remove leftover commas
    simplified.erase(std::remove(simplified.begin(),
                                 simplified.end(), ','), simplified.end());

    return trim(simplified);
}

} // namespace kfetch
	
