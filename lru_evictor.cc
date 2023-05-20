/*
 * Declare interface for eviction policies and a few implementations.
 */

//#pragma once

#include <string>
#include "lru_evictor.hh"


// Inform evictor that a certain key has been set or get:
void Lru_evictor::touch_key(const key_type& key) {
	//If key is present, erase key from list first
	if (hm.find(key) != hm.end()){
		dll.erase(hm[key]);
	}

	//move reference to front
	dll.push_front(key);
	hm[key] = dll.begin();
}

// Request evictor for the next key to evict, and remove it from evictor.
// If evictor doesn't know what to evict, return an empty key ("").
const key_type Lru_evictor::evict() {
	if(!dll.empty()) {
		key_type to_evict = dll.back();
		dll.pop_back();
		hm.erase(to_evict);
		return to_evict;
	}
	return "";
	
}