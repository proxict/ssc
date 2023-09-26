#ifndef PROXICT_SSC_FILELOCK_HPP_
#define PROXICT_SSC_FILELOCK_HPP_

#include "ssc/Exception.hpp"

extern "C" {
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
}

#include <chrono>
#include <cstring>
#include <filesystem>
#include <thread>

namespace ssc::fs {

class FileLock final {
public:
    explicit FileLock(const std::filesystem::path& filePath) {
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

} // namespace ssc::fs

#endif // PROXICT_SSC_FILELOCK_HPP_
