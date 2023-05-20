
#include <algorithm>
#include "cache.hh"
#include <iostream>
#include <unordered_map>
#include <vector>

  // Create a new cache object with the following parameters:
  // maxmem: The maximum allowance for storage used by values.
  // max_load_factor: Maximum allowed ratio between buckets and table rows.
  // evictor: Eviction policy implementation (if nullptr, no evictions occur
  // and new insertions fail after maxmem has been exceeded).
  // hasher: Hash function to use on the keys. Defaults to C++'s std::hash.

class Cache::Impl
{
  public:
    size_type maxmem;
    float max_load_factor;
    size_type curmem;
  	hash_func hasher;
    Evictor* evictor;
    std::unordered_map<key_type, val_type*, hash_func> cache_map; //std::function<size_t(const key_type &key)>
  	std::uint32_t hits;
  	std::uint32_t misses;

    Impl(size_type maxmem,
    float max_load_factor,
    Evictor* evictor,
    hash_func hasher);
    ~Impl();
  private: 
};	

Cache::Impl::Impl(size_type maxmem,
    float max_load_factor,
    Evictor* evictor,
    hash_func hasher)
    : maxmem(maxmem),max_load_factor(max_load_factor),curmem(0),hasher(hasher),
	evictor(evictor),cache_map(0, hasher),hits(0),misses(0)
{ }

Cache::Impl::~Impl() { 
}

Cache::Cache(size_type maxmem,
    float max_load_factor,
    Evictor* evictor,
    hash_func hasher):
	pImpl_(new Impl(maxmem,
    max_load_factor,
	evictor,
	hasher)) 
{ }

Cache::~Cache() {
  reset();
  //delete pImpl_;
}

// Add a <key, value> pair to the cache.
// If key already exists, it will overwrite the old value.
// Both the key and the value are to be deep-copied (not just pointer copied).
// If maxmem capacity is exceeded, enough values will be removed
// from the cache to accomodate the new value. If unable, the new value
// isn't inserted to the cache.
   // Returns true iff the insertion of the data to the store was successful.
bool Cache::set(key_type key, val_type val) {
  //delete old value if it exists
  del(key);

  //if no eviction and not enough space, or size greater than total space, cache overflow
  if((pImpl_ -> curmem + val.size_ > pImpl_ -> maxmem && pImpl_ -> evictor == nullptr) || val.size_ > pImpl_ -> maxmem) {
  } else { 
  
	//evict until enough space, then insert key
    while (pImpl_ -> curmem + val.size_ > pImpl_ -> maxmem) {
      key_type toEvict = (pImpl_ -> evictor) -> evict();
      del(toEvict);
    }
	
	// insert the key
    byte_type *b = new byte_type[val.size_];

    strcpy(b, val.data_);
    val_type *nval = new val_type {b, val.size_};
    pImpl_ -> cache_map[key] = nval;
	
	//resize the cache if the load factor exceeds max_load_factor
	if(pImpl_ -> cache_map.load_factor() > pImpl_ -> max_load_factor) {
		pImpl_ -> cache_map.rehash(2 * pImpl_ -> cache_map.size());
	}
	
    //touch evictor
    if (pImpl_ -> evictor != nullptr) {
      (pImpl_ -> evictor) -> touch_key(key);
    }
	
    //add new size
	pImpl_ -> curmem += val.size_;
    return true;
  }
	return false;
}	


// Retrieve a copy of the value associated with key in the cache,
// or nullptr with size 0 if not found.
// Note that the data_ pointer in the return key is a newly-allocated
// copy of the data. It is the caller's responsibility to free it.
Cache::val_type Cache::get(key_type key) const {
	if((pImpl_ -> cache_map).count(key) == 0) {
		val_type val = val_type {nullptr,0};
		pImpl_ -> misses += 1;
		return val;
	} else {
    //touch evictor
    if (pImpl_ -> evictor != nullptr) {
      (pImpl_ -> evictor) -> touch_key(key);
    } 
	
	
    pImpl_ -> hits += 1;
    val_type old_val = *(pImpl_ -> cache_map.at(key));
    size_type size = old_val.size_;
    byte_type* data = new byte_type[size];

    //perform deep copy
    strcpy(data, old_val.data_);
    val_type val = val_type {data, size};
    return(val);
  }
}

// Delete an object from the cache, if it's still there.
// Returns true iff the object was deleted from the store.
bool Cache::del(key_type key) {
  //get iterator to val
  auto iter = (pImpl_ -> cache_map).find(key);
  if(iter == (pImpl_ -> cache_map).end()) {
    return false;
  } else {
    auto val = iter ->second;
    pImpl_ -> curmem -= val->size_;

    //delete data, not just pointer
    delete[] val->data_;
    delete val;
    (pImpl_ -> cache_map).erase(iter);
    return true;
  }
}

// Compute the total amount of memory used up by all cache values (not keys)
Cache::size_type Cache::space_used() const {
  return pImpl_->curmem;
}

// Return the ratio of successful gets to all gets
double Cache::hit_rate() const {
	if((pImpl_ -> hits) + (pImpl_ -> misses) == 0) {
		return 0.0;
	}
	double hit_rate = (double)(pImpl_ -> hits) / ((pImpl_ -> hits) + (pImpl_ -> misses));
	return hit_rate;
}

// Delete all data from the cache and return true iff successful
bool Cache::reset() {

	pImpl_ -> hits = 0;
	pImpl_ -> misses = 0;
	
	std::vector<key_type> keynames; 
	//get all the key names
	for (auto i : pImpl_ -> cache_map) {
		keynames.push_back(i.first);
	}

	for (auto i : keynames) {
		del(i);
	}

	pImpl_ -> cache_map.clear();
    return true;
}






