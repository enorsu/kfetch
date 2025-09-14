#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

namespace kfetch {

struct Config {
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
    bool verbose_output = false; // default: silent
    
    std::string custom_art_color = "";
    std::string custom_text_color = "";
    
    // Load from config file
    bool loadFromFile(const std::string& path);
    
    // Parse command line arguments
    void parseArgs(int argc, char* argv[]);
    
    // Optional: Config validation
    bool validate() const;
    
    // Optional: Debug printing
    void print() const;
};

} // namespace kfetch

#endif // CONFIG_H
