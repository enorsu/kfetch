#include "config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace kfetch {

// Trim whitespace
std::string trim(const std::string& s) {
    auto first = s.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    auto last = s.find_last_not_of(" \t\n\r");
    return s.substr(first, last - first + 1);
}

// Convert named color to ANSI
std::string colorNameToCode(const std::string& colorName) {
    static const std::unordered_map<std::string, std::string> colors = {
        {"black","\033[0;30m"}, {"red","\033[0;31m"}, {"green","\033[0;32m"},
        {"yellow","\033[0;33m"}, {"blue","\033[0;34m"}, {"magenta","\033[0;35m"},
        {"cyan","\033[0;36m"}, {"white","\033[0;37m"},
        {"bright_black","\033[1;30m"}, {"bright_red","\033[1;31m"}, {"bright_green","\033[1;32m"},
        {"bright_yellow","\033[1;33m"}, {"bright_blue","\033[1;34m"}, {"bright_magenta","\033[1;35m"},
        {"bright_cyan","\033[1;36m"}, {"bright_white","\033[1;37m"},
        {"reset","\033[0m"}
    };
    auto it = colors.find(colorName);
    return it != colors.end() ? it->second : colorName;
}

// Load config file
bool Config::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        if (verbose_output) std::cerr << "Config: Could not open " << path << "\n";
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        auto comment_pos = line.find('#');
        if (comment_pos != std::string::npos) line = line.substr(0, comment_pos);
        line = trim(line);
        if (line.empty()) continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim(line.substr(0, eq));
        std::string value = trim(line.substr(eq + 1));

        // Standard boolean flags
        if (key == "show_art") show_art = (value == "true");
        else if (key == "show_colors") show_colors = (value == "true");
        else if (key == "show_username") show_username = (value == "true");
        else if (key == "show_hostname") show_hostname = (value == "true");
        else if (key == "show_os") show_os = (value == "true");
        else if (key == "show_kernel") show_kernel = (value == "true");
        else if (key == "show_uptime") show_uptime = (value == "true");
        else if (key == "show_packages") show_packages = (value == "true");
        else if (key == "show_shell") show_shell = (value == "true");
        else if (key == "show_de") show_de = (value == "true");
        else if (key == "show_terminal") show_terminal = (value == "true");
        else if (key == "show_cpu") show_cpu = (value == "true");
        else if (key == "show_memory") show_memory = (value == "true");

        // Custom colors
        else if (key == "custom_art_color") custom_art_color = colorNameToCode(value);
        else if (key == "custom_text_color") custom_text_color = colorNameToCode(value);

        // Extra user-defined options
        else extras[key] = value;
    }

    return true;
}

// Parse command-line args
void Config::parseArgs(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--output") verbose_output = true;
        else if (arg == "--no-art") show_art = false;
        else if (arg == "--no-colors") show_colors = false;
        else if (arg == "--no-username") show_username = false;
        else if (arg == "--no-hostname") show_hostname = false;
        else if (arg == "--no-os") show_os = false;
        else if (arg == "--no-kernel") show_kernel = false;
        else if (arg == "--no-uptime") show_uptime = false;
        else if (arg == "--no-packages") show_packages = false;
        else if (arg == "--no-shell") show_shell = false;
        else if (arg == "--no-de") show_de = false;
        else if (arg == "--no-terminal") show_terminal = false;
        else if (arg == "--no-cpu") show_cpu = false;
        else if (arg == "--no-memory") show_memory = false;
    }
}

// Validate config
bool Config::validate() const {
    return true; // can expand to check colors, etc.
}

// Debug printing
void Config::print() const {
    std::cout << "=== Config ===\n";
    std::cout << "show_art=" << show_art << "\n";
    std::cout << "show_colors=" << show_colors << "\n";
    std::cout << "custom_art_color=" << custom_art_color << "\n";
    std::cout << "custom_text_color=" << custom_text_color << "\n";
    for (auto& [k,v] : extras) std::cout << k << "=" << v << "\n";
}

// Extras getters
std::string Config::getExtra(const std::string& key, const std::string& defaultValue) const {
    auto it = extras.find(key);
    return it != extras.end() ? it->second : defaultValue;
}

bool Config::getExtraBool(const std::string& key, bool defaultValue) const {
    auto it = extras.find(key);
    return it != extras.end() ? (it->second == "true") : defaultValue;
}

} // namespace kfetch
