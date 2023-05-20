/*
 * Declare interface for eviction policies and a few implementations.
 */

//#pragma once

#include <string>
#include "fifo_evictor.hh"


// Inform evictor that a certain key has been set or get:
void Fifo_evictor::touch_key(const key_type& key) {
	Q.push(key);
}

// Request evictor for the next key to evict, and remove it from evictor.
// If evictor doesn't know what to evict, return an empty key ("").
const key_type Fifo_evictor::evict() {
	if(!Q.empty()) {
		key_type to_evict = Q.front();
		Q.pop();
		return to_evict;
	}
	return "";
}

// Fifo_evictor::~Evictor() {
	
// }