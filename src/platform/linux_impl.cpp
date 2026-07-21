#include "universal_sdk.h"
#include <unistd.h>
#include <sys/utsname.h>
#include <sstream>

namespace UniversalSDK {

class LinuxImpl : public PlatformImpl {
public:
    bool Initialize() override {
        // Linux-specific initialization
        return true;
    }

    void Shutdown() override {
        // Linux-specific cleanup
    }

    std::string GetSystemInfo() override {
        std::ostringstream info;
        info << "Linux Platform Information:\n";
        
        struct utsname buf;
        if (uname(&buf) == 0) {
            info << "System Name: " << buf.sysname << "\n";
            info << "Node Name: " << buf.nodename << "\n";
            info << "Release: " << buf.release << "\n";
            info << "Version: " << buf.version << "\n";
            info << "Machine: " << buf.machine << "\n";
        }
        
        info << "Number of CPUs: " << sysconf(_SC_NPROCESSORS_ONLN) << "\n";
        info << "Page Size: " << sysconf(_SC_PAGESIZE) << " bytes\n";
        
        return info.str();
    }

    bool ExecuteCommand(const std::string& command) override {
        // Linux command execution
        int result = system(command.c_str());
        return result == 0;
    }
};

std::unique_ptr<PlatformImpl> CreatePlatformImpl() {
    return std::make_unique<LinuxImpl>();
}

} // namespace UniversalSDK
