#pragma once

#include <unordered_map>
#include <list>
#include <cstddef>
#include <stdexcept>

namespace kea {
namespace util {

static const int MAX_CACHE_ITEM_COUNT = 10240;

template<typename key_t, typename value_t>
class LruCache{
public:
	typedef typename std::pair<key_t, value_t> key_value_pair_t;
	typedef typename std::list<key_value_pair_t>::iterator list_iterator_t;

	LruCache(size_t max_size) : max_size_(max_size) {
        if (max_size_ == 0) {
            max_size_ = MAX_CACHE_ITEM_COUNT;
        }
	}
	
	void put(const key_t& key, const value_t& value) {
		auto it = cache_items_map_.find(key);
		cache_items_list_.push_front(key_value_pair_t(key, value));
		if (it != cache_items_map_.end()) {
			cache_items_list_.erase(it->second);
			cache_items_map_.erase(it);
		}
		cache_items_map_[key] = cache_items_list_.begin();
		
		if (cache_items_map_.size() > max_size_) {
			auto last = cache_items_list_.end();
			last--;
			cache_items_map_.erase(last->first);
			cache_items_list_.pop_back();
		}
	}
	
	value_t& get(const key_t& key) {
		auto it = cache_items_map_.find(key);
		if (it == cache_items_map_.end()) {
			throw std::range_error("There is no such key in cache");
		} else {
			cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, it->second);
			return it->second->second;
		}
	}

    bool find(const key_t& key, value_t** value) {
		auto it = cache_items_map_.find(key);
		if (it == cache_items_map_.end()) {
            return false;
		} else {
			cache_items_list_.splice(cache_items_list_.begin(), cache_items_list_, it->second);
            *value = &it->second->second;
            return true;
		}
	}
	
	bool exists(const key_t& key) const {
		return cache_items_map_.find(key) != cache_items_map_.end();
	}
	
	size_t size() const {
		return cache_items_map_.size();
	}
	
private:
	std::list<key_value_pair_t> cache_items_list_;
	std::unordered_map<key_t, list_iterator_t> cache_items_map_;
	size_t max_size_;
};

};
};
