#ifndef PROXICT_SSC_SSC_HPP_
#define PROXICT_SSC_SSC_HPP_

#include "ssc/Filesystem.hpp"

#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/chrono.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <ctime>
#include <fstream>
#include <thread>
#include <typeinfo>
#include <unordered_map>

using namespace std::chrono_literals;

namespace ssc {

enum class WriteMode { ALWAYS, IF_SET, IF_NOT_SET };
enum class DeserializationMode { OVERWRITE, MERGE_UPDATE, MERGE_ONLY_NEW, MERGE_ONLY_EXISTING };

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::time_point<Clock>;

template <typename TKey, typename TValue, std::size_t TShardSize = 32>
class Cache final {
public:
    explicit Cache(std::string basePath)
        : mBasePath(std::move(basePath)) {
        const std::size_t keyTypeHash = typeid(TKey).hash_code();
        const std::size_t valueTypeHash = typeid(TValue).hash_code();
        const std::size_t shardSize = TShardSize;

        const std::string metaFile = mBasePath + "/.meta";
        if (!fs::isDirectory(mBasePath)) {
            fs::createDirectory(mBasePath);
            std::ofstream meta(metaFile, std::ios::binary);
            if (!meta) {
                throw Exception("Failed to write cache metadata: ", strerror(errno));
            }
            meta.write(reinterpret_cast<const char*>(&keyTypeHash), sizeof(std::size_t));
            meta.write(reinterpret_cast<const char*>(&valueTypeHash), sizeof(std::size_t));
            meta.write(reinterpret_cast<const char*>(&shardSize), sizeof(std::size_t));
        } else {
            std::ifstream meta(metaFile, std::ios::binary);
            if (!meta) {
                throw Exception("Failed to read cache metadata: ", strerror(errno));
            }
            std::size_t storedKeyTypeHash = 0;
            std::size_t storedValueTypeHash = 0;
            std::size_t storedShardSize = 0;

            meta.read(reinterpret_cast<char*>(&storedKeyTypeHash), sizeof(std::size_t));
            meta.read(reinterpret_cast<char*>(&storedValueTypeHash), sizeof(std::size_t));
            meta.read(reinterpret_cast<char*>(&storedShardSize), sizeof(std::size_t));

            if (storedKeyTypeHash != keyTypeHash || storedValueTypeHash != valueTypeHash ||
                shardSize != storedShardSize) {
                throw Exception(
                    "Trying to read incompatible cache - key type, value type, and/or shard size differs");
            }
            if (!deserialize(DeserializationMode::OVERWRITE)) {
                throw Exception("Trying to read incompatible cache - inconsistent database");
            }
        }
    }

    std::size_t getSize() const {
        std::size_t total = 0;
        std::for_each(
            std::begin(mShards), std::end(mShards), [&total](const Shard& s) { total += s.getSize(); });
        return total;
    }

    bool isEmpty() const { return getSize() == 0; }

    bool storeValue(TKey key,
                    TValue value,
                    const TimePoint expiryTime = TimePoint::max(),
                    const WriteMode writeMode = WriteMode::ALWAYS) {
        Shard& shard = getShard(key);
        return shard.store(std::move(key), std::move(value), expiryTime, writeMode);
    }

    bool storeValue(TKey key, TValue value, const WriteMode writeMode) {
        return storeValue(std::move(key), std::move(value), TimePoint::max(), writeMode);
    }

    const TValue* getValue(const TKey& key) const {
        const Shard& shard = getShard(key);
        return shard.getValue(key);
    }

    template <typename TFunctor>
    void doForAll(const TFunctor& functor) const {
        std::for_each(
            std::begin(mShards), std::end(mShards), [&functor](const Shard& s) { s.doForAll(functor); });
    }

    bool erase(const TKey& key) {
        Shard& shard = getShard(key);
        return shard.erase(key);
    }

    void clear() {
        std::for_each(std::begin(mShards), std::end(mShards), [](Shard& s) { s.clear(); });
    }

    const TimePoint* getExpiryTime(const TKey& key) const {
        const Shard& shard = getShard(key);
        return shard.getExpiryTime(key);
    }

    std::size_t curate() {
        std::size_t totalExpired = 0;
        std::for_each(std::begin(mShards), std::end(mShards), [&totalExpired](Shard& s) {
            totalExpired += s.eraseExpiredEntries();
        });
        return totalExpired;
    }

    bool serialize() const {
        bool serializedAll = true;
        for (std::size_t i = 0; i < TShardSize; ++i) {
            const Shard& shard = mShards.at(i);
            const auto shardFilename = getShardFilename(i);
            if (fs::isFile(shardFilename) && !shard.isDirty()) {
                continue;
            }

            std::ofstream shardFile(shardFilename, std::ios::binary);
            if (!shardFile) {
                serializedAll = false;
                continue;
            }
            shard.serialize(shardFile);
            shardFile.close();
        }
        return serializedAll;
    }

    bool deserialize(const DeserializationMode mode = DeserializationMode::OVERWRITE) {
        bool deserializedAll = true;
        for (std::size_t i = 0; i < TShardSize; ++i) {
            const std::string shardFilename = getShardFilename(i);
            std::ifstream shardFile(shardFilename, std::ios::binary);
            if (!shardFile) {
                deserializedAll = false;
                continue;
            }
            Shard& shard = mShards.at(i);
            shard.deserialize(shardFile, mode);
        }
        return deserializedAll;
    }

private:
    class Entry final {
    public:
        explicit Entry()
            : mValue()
            , mExpiryTime(TimePoint::max()) {}

        explicit Entry(TValue value, const TimePoint expiryTime)
            : mValue(std::move(value))
            , mExpiryTime(expiryTime) {}

        bool isExpired() const { return mExpiryTime != TimePoint::max() && Clock::now() > mExpiryTime; }

        const TValue& getValue() const { return mValue; }

        const TimePoint& getExpiryTime() const { return mExpiryTime; }

        template <class Archive>
        void serialize(Archive& archiver) {
            archiver(mValue, mExpiryTime);
        }

    private:
        TValue mValue;
        TimePoint mExpiryTime;
    };

    class Shard {
    public:
        explicit Shard() = default;

        std::size_t getSize() const { return mStorage.size(); }

        bool store(TKey key, TValue value, const TimePoint expiryTime, const WriteMode writeMode) {
            const auto it = mStorage.find(key);
            const bool keyExists = it != mStorage.end();

            switch (writeMode) {
            case WriteMode::ALWAYS:
                break;
            case WriteMode::IF_NOT_SET:
                if (keyExists) {
                    return false;
                }
                break;
            case WriteMode::IF_SET:
                if (!keyExists) {
                    return false;
                }
            default:
                break;
            }

            if (keyExists) {
                it->second = Entry(std::move(value), expiryTime);
            } else {
                mStorage.emplace(std::piecewise_construct,
                                 std::forward_as_tuple(key),
                                 std::forward_as_tuple(std::move(value), expiryTime));
            }
            mDirty = true;
            return true;
        }

        const TValue* getValue(const TKey& key) const {
            const auto it = mStorage.find(key);

            if (it == mStorage.end()) {
                return nullptr;
            }

            const Entry& entry = it->second;
            if (entry.isExpired()) {
                return nullptr;
            }
            return &entry.getValue();
        }

        template <typename TFunctor>
        void doForAll(const TFunctor& functor) const {
            for (const auto& entry : mStorage) {
                const TValue* value = getValue(entry.first);
                if (value) {
                    functor(entry.first, *value);
                }
            }
        }

        const TimePoint* getExpiryTime(const TKey& key) const {
            const auto it = mStorage.find(key);
            if (it == mStorage.end()) {
                return nullptr;
            }
            const Entry& entry = it->second;
            return &entry.getExpiryTime();
        }

        bool erase(const TKey& key) {
            const bool erased = mStorage.erase(key) > 0;
            if (erased) {
                mDirty = true;
            }
            return erased;
        }

        void clear() { mStorage.clear(); }

        std::size_t eraseExpiredEntries() {
            std::size_t totalExpired = 0;
            for (auto it = mStorage.begin(); it != mStorage.end();) {
                const bool isExpired = it->second.isExpired();
                it = isExpired ? mStorage.erase(it) : std::next(it);
                if (isExpired) {
                    ++totalExpired;
                }
            }
            if (totalExpired != 0) {
                mDirty = true;
            }
            return totalExpired;
        }

        std::ostream& serialize(std::ostream& stream) const {
            cereal::BinaryOutputArchive archiver(stream);
            archiver(mStorage);
            mDirty = false;
            return stream;
        }

        std::istream& deserialize(std::istream& stream, const DeserializationMode mode) {
            cereal::BinaryInputArchive dearchiver(stream);
            if (mode == DeserializationMode::OVERWRITE) {
                dearchiver(mStorage);
            } else {
                std::unordered_map<TKey, Entry> intermediate;
                dearchiver(intermediate);
                for (const auto& entry : intermediate) {
                    if (mStorage.count(entry.first) > 0) {
                        if (mode == DeserializationMode::MERGE_ONLY_NEW) {
                            continue;
                        }
                    } else {
                        if (mode == DeserializationMode::MERGE_ONLY_EXISTING) {
                            continue;
                        }
                    }
                    mStorage[entry.first] = entry.second;
                    mDirty = true;
                }
            }
            return stream;
        }

        bool isDirty() const { return mDirty; }

    private:
        std::unordered_map<TKey, Entry> mStorage;
        mutable bool mDirty = false;
    };

    std::size_t getShardId(const TKey& key) const { return mKeyHasher(key) % TShardSize; }

    const Shard& getShard(const TKey& key) const { return mShards.at(getShardId(key)); }

    Shard& getShard(const TKey& key) { return mShards.at(getShardId(key)); }

    std::string getShardFilename(const std::size_t shardIndex) const {
        return mBasePath + ("/shard" + std::to_string(shardIndex));
    }

    std::hash<TKey> mKeyHasher;
    std::array<Shard, TShardSize> mShards;
    std::string mBasePath;
};

} // namespace ssc

#endif // PROXICT_SSC_SSC_HPP_
