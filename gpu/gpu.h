#ifndef GPU_H
#define GPU_H

#include <string>

namespace kfetch {

class GPUInfo {
private:
    std::string gpu_name;
    std::string driver_version;
    
public:
    GPUInfo();
    const std::string& getName() const { return gpu_name; }
    const std::string& getDriverVersion() const { return driver_version; }
    std::string getFormatted() const;
};

} // namespace kfetch

#endif // GPU_H
