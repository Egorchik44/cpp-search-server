#pragma once

#include <mutex>
#include <map>
#include <vector>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {


public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

struct VK {
        std::mutex mutex;
        std::map<Key, Value> map;
    };

    struct Access {
        std::lock_guard <std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, VK& bucket)
            : guard(bucket.mutex),
            ref_to_value(bucket.map[key])
        {
        }
    };

    explicit ConcurrentMap(size_t bucket_count)
        : buckets_(bucket_count)
    {
    }

    Access operator[](const Key& key) {
        auto& bucket = buckets_[key % buckets_.size()];
        return { key, bucket };
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [mutex, map] : buckets_) {
            std::lock_guard guard(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }
    
    auto Erase(const Key& key) { 
		uint64_t tmp_key = static_cast<uint64_t>(key) % buckets_.size(); 
		std::lock_guard guard(buckets_[tmp_key].mutex); 
		return buckets_[tmp_key].map.erase(key); 
	}

private:
    

    std::vector<VK> buckets_;
};
