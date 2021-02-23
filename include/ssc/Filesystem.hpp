#ifndef PROXICT_SSC_FILESYSTEM_HPP_
#define PROXICT_SSC_FILESYSTEM_HPP_

#include <sys/types.h>
#include <unistd.h>

extern "C" {
#include <dirent.h>
#include <sys/stat.h>
}

#include "ssc/Exception.hpp"

#include <cstring>
#include <string>

namespace ssc {
namespace fs {

    inline bool isFile(const std::string& file) {
        struct ::stat st;
        return ::stat(file.c_str(), &st) == 0 && S_ISREG(st.st_mode);
    }

    inline bool isDirectory(const std::string& directory) {
        struct stat st;
        return stat(directory.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
    }

    inline void createDirectory(const std::string& directory) {
        if (::mkdir(directory.c_str(), S_IRWXU) == -1) {
            throw Exception("Failed to create directory: ", std::strerror(errno));
        }
    }

} // namespace fs
} // namespace ssc

#endif // PROXICT_SSC_FILESYSTEM_HPP_
