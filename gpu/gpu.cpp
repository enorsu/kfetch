#include "gpu.h"
#include "utils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <string_view>
#include <ranges>

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
    // Try lspci first
    if (system("command -v lspci >/dev/null 2>&1") == 0) {
        std::string out = runCommand(
            "lspci -v | grep -A 10 'VGA\\|3D' | "
            "grep -E 'VGA|3D|NVIDIA|AMD|Intel' | head -1");
        if (!out.empty()) {
            if (auto colon = out.find(": "); colon != std::string::npos)
                gpu_name = trim(out.substr(colon + 2));
            else
                gpu_name = trim(out);
        }
    }

    // NVIDIA-specific query fallback
    if (gpu_name.empty() &&
        system("command -v nvidia-smi >/dev/null 2>&1") == 0) {
        gpu_name = trim(runCommand(
            "nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null"));
    }

#elif defined(__FreeBSD__) || defined(__DragonFly__)
    // NVIDIA query FIRST for FreeBSD
    // NVIDIA query first
    if (system("command -v nvidia-smi >/dev/null 2>&1") == 0) {
        gpu_name = trim(runCommand(
            "nvidia-smi --query-gpu=name --format=csv,noheader 2>/dev/null"));
    }

    // pciconf fallback
    if (gpu_name.empty()) {
        std::string output = runCommand(
            "pciconf -lv | grep -i -A 3 -E '(vgapci|nvidia|amd|radeon|intel)'");

        auto extractValue = [](std::string_view line, std::string_view key) -> std::string {
            if (auto pos = line.find(key); pos != std::string_view::npos)
                return std::string(line.substr(pos + key.size()));
            return {};
        };

        std::string vendor, device;
        for (auto line_view : std::views::split(output, '\n')) {
            std::string_view line(&*line_view.begin(), std::ranges::distance(line_view));
            if (line.starts_with("vendor = '"))
                vendor = extractValue(line, "vendor = '");
            if (line.starts_with("device = '"))
                device = extractValue(line, "device = '");
        }

        if (!vendor.empty() || !device.empty())
            gpu_name = trim(vendor + " " + device);
    }

    // dmesg fallback
    if (gpu_name.empty())
        gpu_name = trim(runCommand(
            "dmesg | grep -i -E '(nvidia|amd|radeon|intel).*graphics|vga' | head -1"));

    // OpenGL fallback
    if (gpu_name.empty() &&
        system("command -v glxinfo >/dev/null 2>&1") == 0) {
        gpu_name = trim(runCommand(
            "glxinfo 2>/dev/null | grep 'OpenGL renderer string' | cut -d: -f2"));
    }

#elif defined(__OpenBSD__) || defined(__NetBSD__)
    gpu_name = trim(runCommand(
        "dmesg | grep -i 'vga\\|graphics\\|nvidia\\|amd\\|radeon' | head -1"));
#endif

    if (gpu_name.empty())
        gpu_name = "Unknown GPU";
}

std::string GPUInfo::getFormatted() const {
    if (gpu_name == "Unknown GPU") return gpu_name;

    std::string simplified = gpu_name;

    // Quick string replacements
    static const std::unordered_map<std::string_view, std::string_view> replacements = {
        {"Advanced Micro Devices", "AMD"},
        {"Intel Corporation", "Intel"},
        {"NVIDIA Corporation", "NVIDIA"},
        {"Corporation", ""},
        {"Inc.", ""}
    };

    for (auto [from, to] : replacements) {
        size_t pos = 0;
        while ((pos = simplified.find(from, pos)) != std::string::npos) {
            simplified.replace(pos, from.size(), std::string(to));
            pos += to.size();
        }
    }

    // Remove known prefixes
    for (auto prefix : {"vendor ", "device "}) {
        if (simplified.starts_with(prefix))
            simplified.erase(0, std::char_traits<char>::length(prefix));
    }

    // Remove quotes, commas, and brackets
    std::erase_if(simplified, [](char c) { return c == '\'' || c == '\"' || c == ',' || c == '[' || c == ']'; });

    // Collapse multiple spaces using ranges
    std::string result;
    bool lastSpace = false;
    for (char c : simplified) {
        if (std::isspace(c)) {
            if (!lastSpace) { result += ' '; lastSpace = true; }
        } else {
            result += c;
            lastSpace = false;
        }
    }

    return trim(result);
}

} // namespace kfetch
