#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unordered_map>

namespace kfetch {

struct Config {
    // Display toggles
    bool show_art = true;
    bool show_colors = true;
    bool show_username = true;
    bool show_hostname = true;
    bool show_os = true;
    bool show_kernel = true;
    bool show_uptime = true;
    bool show_packages = true;
    bool show_shell = true;
    bool show_de = true;
    bool show_terminal = true;
    bool show_cpu = true;
    bool show_memory = true;

    // Custom colors
    std::string custom_art_color = "";
    std::string custom_text_color = "";

    // Verbose output
    bool verbose_output = false;

    // Any additional settings from config file
    std::unordered_map<std::string, std::string> extras;

    // Load config file
    bool loadFromFile(const std::string& path);

    // Parse CLI arguments
    void parseArgs(int argc, char* argv[]);

    // Validate config
    bool validate() const;

    // Debug print
    void print() const;

    // Get a string value from extras, fallback default
    std::string getExtra(const std::string& key, const std::string& defaultValue="") const;

    // Get a bool value from extras, fallback default
    bool getExtraBool(const std::string& key, bool defaultValue=false) const;
};

// Utility
std::string colorNameToCode(const std::string& colorName);

} // namespace kfetch

#endif // CONFIG_H
