#ifndef PROXICT_SSC_FILESYSTEM_HPP_
#define PROXICT_SSC_FILESYSTEM_HPP_

#include <sys/types.h>
#include <unistd.h>

extern "C" {
#include <dirent.h>
#include <ftw.h>
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

    inline void removeDirectory(const std::string& directory) {
        if (directory == "." || directory == ".." || directory == "/") {
            throw Exception("Refusing to remove directory ", directory);
        }

        auto unlinkCallback = [](const char* fpath, const struct stat* sb, int typeFlag, struct FTW* ftwBuf) {
            (void)sb;
            (void)typeFlag;
            (void)ftwBuf;
            return remove(fpath);
        };

        if (nftw(directory.c_str(), unlinkCallback, 64, FTW_DEPTH | FTW_PHYS) == -1) {
            throw Exception("Failed to remove directory ", directory, ":", std::strerror(errno));
        }
    }

} // namespace fs
} // namespace ssc

#endif // PROXICT_SSC_FILESYSTEM_HPP_
