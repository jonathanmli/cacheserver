#ifndef FIFO_EVICTOR_HH
#define FIFO_EVICTOR_HH

/*
 * Declare interface for eviction policies and a few implementations.
 */

//#pragma once

#include <string>
#include "evictor.hh"
#include <queue>

class Fifo_evictor: public Evictor{
public:
	// Evictor() = default;
	// ~Evictor() = default;

	// Inform evictor that a certain key has been set or get:
	void touch_key(const key_type&);

	// Request evictor for the next key to evict, and remove it from evictor.
	// If evictor doesn't know what to evict, return an empty key ("").
	const key_type evict();
private:
	std::queue<key_type> Q; 
};

#endif