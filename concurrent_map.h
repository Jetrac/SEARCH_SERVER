#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include <mutex>

template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct Bucket {
        std::mutex mutex;
        std::map<Key, Value> map;
    };

    std::vector<Bucket> buckets_;
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket): guard(bucket.mutex), ref_to_value(bucket.map[key]) {
        }
    };
    void ERASE(const Key& key) {
        int bucket_index = static_cast<uint64_t>(key) % buckets_.size();
        std::lock_guard a(buckets_.at(bucket_index).mutex);
        buckets_.at(bucket_index).map.erase(key);
    }

    explicit ConcurrentMap(size_t bucket_count): buckets_(bucket_count) {
    }

    Access operator[](const Key& key) {
        auto& bucket = buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        return {key, bucket};
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [mutex_, map_] : buckets_) {
            std::lock_guard g(mutex_);
            result.insert(map_.begin(), map_.end());
        }
        return result;
    }
};
