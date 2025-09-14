#include "config.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>

namespace kfetch {

// Helper function to trim whitespace
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

// Helper function to parse escape sequences
std::string parseEscapeSequences(const std::string& input) {
    std::string result;
    size_t pos = 0;
    size_t last_pos = 0;
    
    while ((pos = input.find("\\033", last_pos)) != std::string::npos) {
        // Add the text before the escape sequence
        result += input.substr(last_pos, pos - last_pos);
        
        // Add the actual escape character
        result += "\033";
        
        // Skip the "\033" part
        last_pos = pos + 4;
        
        // Check if there's a style code after the escape sequence
        if (last_pos < input.length() && input[last_pos] == '[') {
            size_t end_style = input.find('m', last_pos);
            if (end_style != std::string::npos) {
                result += input.substr(last_pos, end_style - last_pos + 1);
                last_pos = end_style + 1;
            }
        }
    }
    
    // Add the remaining text
    result += input.substr(last_pos);
    
    return result;
}

// Convert named colors to ANSI escape codes
std::string colorNameToCode(const std::string& colorName) {
    static const std::unordered_map<std::string, std::string> colorMap = {
        {"black", "\033[0;30m"},
        {"red", "\033[0;31m"},
        {"green", "\033[0;32m"},
        {"yellow", "\033[0;33m"},
        {"blue", "\033[0;34m"},
        {"magenta", "\033[0;35m"},
        {"cyan", "\033[0;36m"},
        {"white", "\033[0;37m"},
        {"bright_black", "\033[1;30m"},
        {"bright_red", "\033[1;31m"},
        {"bright_green", "\033[1;32m"},
        {"bright_yellow", "\033[1;33m"},
        {"bright_blue", "\033[1;34m"},
        {"bright_magenta", "\033[1;35m"},
        {"bright_cyan", "\033[1;36m"},
        {"bright_white", "\033[1;37m"},
        {"reset", "\033[0m"},
        // Alternative names
        {"purple", "\033[0;35m"},
        {"bright_purple", "\033[1;35m"},
        {"gray", "\033[0;37m"},
        {"grey", "\033[0;37m"},
        {"bright_gray", "\033[1;37m"},
        {"bright_grey", "\033[1;37m"}
    };
    
    auto it = colorMap.find(colorName);
    if (it != colorMap.end()) {
        return it->second;
    }
    
    // If not a named color, try to parse as escape sequence
    if (colorName.find("\\033") != std::string::npos) {
        return parseEscapeSequences(colorName);
    }
    
    // Return as-is (might be raw ANSI code)
    return colorName;
}

bool Config::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        if (verbose_output) {
            std::cerr << "Config: Could not open file: " << path << std::endl;
        }
        return false;
    }

    if (verbose_output) {
        std::cout << "Config: Loading from " << path << std::endl;
    }

    std::string line;
    int lineNum = 0;
    while (std::getline(file, line)) {
        lineNum++;
        size_t comment_pos = line.find('#');
        if (comment_pos != std::string::npos) line = line.substr(0, comment_pos);
        line = trim(line);
        if (line.empty()) continue;

        size_t equals_pos = line.find('=');
        if (equals_pos == std::string::npos) continue;

        std::string key = trim(line.substr(0, equals_pos));
        std::string value = trim(line.substr(equals_pos + 1));

        if (verbose_output) {
            std::cout << "Config: Parsed - " << key << " = " << value << std::endl;
        }

        // Parse config
        if (key == "show_art") show_art = (value == "true" || value == "1" || value == "yes");
        else if (key == "show_colors") show_colors = (value == "true" || value == "1" || value == "yes");
        else if (key == "show_username") show_username = (value == "true" || value == "1" || value == "yes");
        else if (key == "show_hostname") show_hostname = (value == "true" || value == "1" || value == "yes");
        else if (key == "show_os") show_os = (value == "true" || value == "1" || value == "yes");
        else if (key == "show_kernel") show_kernel = (value == "true" || value == "1" || value == "yes");
        else if (key == "show_uptime") show_uptime = (value == "true" || value == "1" || value == "yes");
        else if (key == "show_packages") show_packages = (value == "true" || value == "1" || value == "yes");
        else if (key == "show_shell") show_shell = (value == "true" || value == "1" || value == "yes");
        else if (key == "show_de") show_de = (value == "true" || value == "1" || value == "yes");
        else if (key == "show_terminal") show_terminal = (value == "true" || value == "1" || value == "yes");
        else if (key == "show_cpu") show_cpu = (value == "true" || value == "1" || value == "yes");
        else if (key == "show_memory") show_memory = (value == "true" || value == "1" || value == "yes");
        else if (key == "custom_art_color") custom_art_color = colorNameToCode(value);
        else if (key == "custom_text_color") custom_text_color = colorNameToCode(value);
    }

    if (verbose_output) {
        std::cout << "Config: Loaded successfully" << std::endl;
    }

    return true;
}

void Config::parseArgs(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        // Enable config verbose output
        if (arg == "--output") {
            verbose_output = true;

        // Disable various sections
        } else if (arg == "--no-art") show_art = false;
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

        // Help message
        else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: kfetch [options]\n"
                      << "Options:\n"
                      << "  --output         Enable config output\n"
                      << "  --no-art         Hide ASCII art\n"
                      << "  --no-colors      Disable colors\n"
                      << "  --no-username    Hide username\n"
                      << "  --no-hostname    Hide hostname\n"
                      << "  --no-os          Hide OS info\n"
                      << "  --no-kernel      Hide kernel info\n"
                      << "  --no-uptime      Hide uptime\n"
                      << "  --no-packages    Hide package count\n"
                      << "  --no-shell       Hide shell info\n"
                      << "  --no-de          Hide desktop environment\n"
                      << "  --no-terminal    Hide terminal info\n"
                      << "  --no-cpu         Hide CPU info\n"
                      << "  --no-memory      Hide memory info\n"
                      << "  --help           Show this help\n";
            exit(0);
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            std::cerr << "Use --help for usage information." << std::endl;
            exit(1);
        }
    }
}

// Optional: Add config validation
bool Config::validate() const {
    // Check if custom colors contain valid ANSI sequences
    auto isValidAnsi = [](const std::string& str) {
        return str.empty() || 
               (str.find("\033[") == 0 && str.find('m') != std::string::npos);
    };
    
    if (!isValidAnsi(custom_art_color)) {
        std::cerr << "Warning: custom_art_color doesn't look like a valid ANSI sequence" << std::endl;
    }
    
    if (!isValidAnsi(custom_text_color)) {
        std::cerr << "Warning: custom_text_color doesn't look like a valid ANSI sequence" << std::endl;
    }
    
    return true;
}

// Optional: Add debug printing
void Config::print() const {
    std::cout << "=== Config Values ===" << std::endl;
    std::cout << "show_art: " << (show_art ? "true" : "false") << std::endl;
    std::cout << "show_colors: " << (show_colors ? "true" : "false") << std::endl;
    std::cout << "show_username: " << (show_username ? "true" : "false") << std::endl;
    std::cout << "show_hostname: " << (show_hostname ? "true" : "false") << std::endl;
    std::cout << "show_os: " << (show_os ? "true" : "false") << std::endl;
    std::cout << "show_kernel: " << (show_kernel ? "true" : "false") << std::endl;
    std::cout << "show_uptime: " << (show_uptime ? "true" : "false") << std::endl;
    std::cout << "show_packages: " << (show_packages ? "true" : "false") << std::endl;
    std::cout << "show_shell: " << (show_shell ? "true" : "false") << std::endl;
    std::cout << "show_de: " << (show_de ? "true" : "false") << std::endl;
    std::cout << "show_terminal: " << (show_terminal ? "true" : "false") << std::endl;
    std::cout << "show_cpu: " << (show_cpu ? "true" : "false") << std::endl;
    std::cout << "show_memory: " << (show_memory ? "true" : "false") << std::endl;
    std::cout << "custom_art_color: " << custom_art_color << std::endl;
    std::cout << "custom_text_color: " << custom_text_color << std::endl;
}

} // namespace kfetch
