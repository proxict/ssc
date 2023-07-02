#ifndef PROXICT_SSC_FILESYSTEM_HPP_
#define PROXICT_SSC_FILESYSTEM_HPP_

#include <sys/types.h>
#include <unistd.h>

extern "C" {
#include <dirent.h>
#include <ftw.h>
#include <sys/file.h>
#include <sys/stat.h>
}

#include "ssc/Exception.hpp"

#include <chrono>
#include <cstring>
#include <string>
#include <thread>
#include <utility>

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

    class FileLock final {
    public:
        explicit FileLock(const std::string& filePath) {
            mLockFd = ::open(filePath.c_str(), O_CREAT, ACCESSPERMS);
            if (mLockFd == -1) {
                throw Exception("Failed to open lock-file ", filePath, ":", std::strerror(errno));
            }
        }

        ~FileLock() noexcept {
            if (mLockFd != 0) {
                unlock();
                ::close(mLockFd);
            }
        }

        FileLock(FileLock&& other) noexcept
            : mLockFd{ std::exchange(other.mLockFd, 0) } {}

        FileLock& operator=(FileLock&& other) noexcept {
            if (this != &other) {
                ::close(mLockFd);
                mLockFd = std::exchange(other.mLockFd, 0);
            }
            return *this;
        }

        void lock() {
            if (::flock(mLockFd, LOCK_EX) == -1) {
                throw Exception("Failed to lock file: ", std::strerror(errno));
            }
        }

        template <typename TRep, typename TPeriod>
        bool lock(const std::chrono::duration<TRep, TPeriod> timeout) noexcept {
            const auto deadline = std::chrono::steady_clock::now() + timeout;
            bool success = tryLock();
            while (!success && std::chrono::steady_clock::now() < deadline) {
                std::this_thread::sleep_for(timeout / 1000);
                success = tryLock();
            }
            return success;
        }

        bool tryLock() noexcept { return ::flock(mLockFd, LOCK_EX | LOCK_NB) == 0; }

        void unlock() {
            if (::flock(mLockFd, LOCK_UN) == -1) {
                throw Exception("Failed to unlock file: ", std::strerror(errno));
            }
        }

    private:
        int mLockFd;
    };

} // namespace fs
} // namespace ssc

#endif // PROXICT_SSC_FILESYSTEM_HPP_
