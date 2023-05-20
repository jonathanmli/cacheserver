/*
 * Interface for a generic cache object.
 * Data is given as blobs (void *) of a given size, and indexed by a string key.
 */
#define CATCH_CONFIG_MAIN
#include "cache.hh"
#include <assert.h>
#include <iostream> 
#include <chrono>
#include "catch.hpp"
//#include <catch2/catch.hpp>

void fill_array(char *test_array, unsigned array_size) {
  for (unsigned i = 0; i < array_size-1; i++){
    test_array[i] = i % 255 +1;
  }
  test_array[array_size-1] = '\0';
}

//make the hasher
//the hash function is defined here as an std::function,
//and st stored in hf to be passed to a cache constructor 
//later.

size_t hf_fast(key_type key) {
	size_t hashed = 0;
	for(auto j = 0; j < (int)key.size(); j++) {
		hashed += 17 * (int)key[j] % 3;
	}
	return hashed;
}

size_t hf_slow(key_type key) {
	size_t hashed = 0;
	for(auto i = 0; i < 1000000; i++) {
		for(auto j = 0; j < (int)key.size(); j++) {
			hashed += 17 * (int)key[j] % 3;
		}
 	}
	return hashed;
}

//make the test objects

Cache::hash_func fast = hf_fast;
Cache::hash_func slow = hf_slow;

Cache test_cache {200,0.75,nullptr,std::hash<key_type>()};
Cache fast_cache {200,0.75,nullptr,hf_fast};
Cache slow_cache {200,0.75,nullptr,hf_slow};

TEST_CASE("Empty get","[cache]") {
	SECTION("Get returns nullptr for an unassigned string") {
		Cache::val_type check_value = test_cache.get("a");
		REQUIRE(check_value.data_ == nullptr);
		delete[] check_value.data_;
	}
}
	
TEST_CASE("Basic set", "[cache]") {
  unsigned array_size = 5;
  char *test_array = new char[array_size];
  std::string keystr = "a";

  fill_array(test_array, array_size);
  Cache::val_type test_value {test_array, array_size};

  SECTION("New value can be set") {
	bool res = test_cache.set(keystr, test_value);
	Cache::val_type check_value = test_cache.get(keystr);
    REQUIRE((res or (check_value.data_ == nullptr)));
	
    delete[] check_value.data_;
	test_cache.reset();
  }

  SECTION("Second value can be set") {
	  
	keystr = "a";
	test_cache.set(keystr, test_value);
    array_size = 2;
    test_array = new char[array_size];
    fill_array(test_array, array_size);
    Cache::val_type test_value {test_array, array_size};
    keystr = "b";
	
    bool res = test_cache.set(keystr, test_value);
	Cache::val_type check_value = test_cache.get(keystr);
    REQUIRE((res or (check_value.data_ == nullptr)));
	
    delete[] test_value.data_;
	delete[] check_value.data_;
	test_cache.reset();
  }

  SECTION("Old value can be replaced") {
    keystr = "a";
    array_size = 7;
    test_array = new char[array_size];
    fill_array(test_array, array_size);
    Cache::val_type test_value {test_array, array_size};
	
    bool res = test_cache.set(keystr, test_value);
	Cache::val_type check_value = test_cache.get(keystr);
    REQUIRE((res or (check_value.data_ == nullptr)));
	
    delete[] test_value.data_;
	delete[] check_value.data_;
	test_cache.reset();
  }
	
	test_cache.reset();
  
}

TEST_CASE("Basic get", "[cache]") {

  // std::cout << "hi" << std::endl;
	
	unsigned array_size = 10;
	char *test_array = new char[array_size];
	std::string keystr = "";
	fill_array(test_array,array_size);
	Cache::val_type test_value {test_array, array_size};
	test_cache.set("a",test_value);
	
	SECTION("get retrieves the correct value") {
		Cache::val_type check_value = test_cache.get("a");
		REQUIRE(check_value.size_ == array_size);
		for(unsigned i = 0; i < array_size; i++) {
			REQUIRE(check_value.data_[i] == test_value.data_[i]);
		}
		
		delete[] test_value.data_;
		delete[] check_value.data_;
		test_cache.reset();
	}

  	test_cache.reset();
}

TEST_CASE("Deep copy", "[cache]") {
	unsigned array_size = 10;
	char *test_array = new char[array_size];
	fill_array(test_array,array_size);
	Cache::val_type test_value {test_array, array_size};
	test_cache.set("a",test_value);
	
	
	SECTION("set performs a deep copy") {
		Cache::val_type check_value = test_cache.get("a");
		REQUIRE(check_value.size_ == array_size);
		for(unsigned i = 0; i < array_size; i++) {
			test_array[i] += 1;
		}
		for(unsigned i = 0; i < array_size; i++) {
			REQUIRE(check_value.data_[i] != test_value.data_[i]);
		}
		
		delete[] check_value.data_;
		delete[] test_value.data_;
		test_cache.reset();
	}

  SECTION("get performs a deep copy") {
    Cache::val_type check_value = test_cache.get("a");
    REQUIRE(check_value.size_ == array_size);
    test_cache.del("a");
    REQUIRE(check_value.data_ != nullptr);
	
    delete[] check_value.data_;
	test_cache.reset();
  }
	test_cache.reset();
}

TEST_CASE("Basic overflow", "[cache]") {
	unsigned array_size = 100;
	char *test_array = new char[array_size];
	std::string keystr = "a";

	fill_array(test_array, array_size);
	Cache::val_type test_value {test_array, array_size};

  test_cache.set(keystr, test_value);
	delete[] test_value.data_;


	SECTION("Do not insert new value if overflow") {
    keystr = "b";
		array_size = 101;
		test_array = new char[array_size];
		fill_array(test_array, array_size);
		Cache::val_type test_value {test_array, array_size};
		
		REQUIRE(!test_cache.set(keystr, test_value));
		
		delete[] test_value.data_;
		test_cache.reset();
	}

	SECTION("Do not insert replacement value if overflow") {
		keystr = "a";
		array_size = 10;
		char* init_array = new char[array_size];
		fill_array(init_array, array_size);
		Cache::val_type init_value {init_array, array_size};
		
		test_cache.set(keystr, init_value);
			
		array_size = 201;
		test_array = new char[array_size];
		fill_array(test_array, array_size);
		Cache::val_type test_value {test_array, array_size};
		
		REQUIRE(!test_cache.set(keystr, test_value));
		
		delete[] test_value.data_;
		delete[] init_value.data_;
		test_cache.reset();
	}

	SECTION("Inserts replacement value if it does not cause overflow") {
		keystr = "a";
		array_size = 10;
		char* init_array = new char[array_size];
		fill_array(init_array, array_size);
		Cache::val_type init_value {init_array, array_size};
		
		test_cache.set(keystr, init_value);
		
		keystr = "a";
		array_size = 199;
		test_array = new char[array_size];
		fill_array(test_array, array_size);
		Cache::val_type test_value {test_array, array_size};
		
		REQUIRE(test_cache.set(keystr, test_value));
		
		delete[] test_value.data_;
		test_cache.reset();
	}

	test_cache.reset();
}

TEST_CASE("Hit rate", "[cache]") {

	//create objects
	Cache::size_type hit_guess = 0;
	Cache::size_type miss_guess = 0;
	double hit_rate_guess = 0;
	std::string keystr = "";
	const unsigned NUM_OBJ = 10; 
	unsigned array_size = 0;
	
	REQUIRE(test_cache.hit_rate() == hit_rate_guess);

	for (unsigned i = 0; i < NUM_OBJ; i++) {
		array_size = 2*i + 2;
		char *test_array = new char[array_size];
		fill_array(test_array, array_size);
		Cache::val_type test_value {test_array, array_size};
		keystr = std::to_string(array_size);
		test_cache.set(keystr, test_value);
		delete[] test_value.data_;
	}
		
	for (unsigned i = 0; i < 2 * NUM_OBJ; i++) {
		array_size = i + 2;
		keystr = std::to_string(array_size);
		Cache::val_type test_value = test_cache.get(keystr);
		if(i % 2 == 0) {hit_guess += 1;}
		else {miss_guess += 1;}
		hit_rate_guess = (double)hit_guess / (hit_guess + miss_guess);
		delete[] test_value.data_;
	}
	
	REQUIRE(test_cache.hit_rate() == hit_rate_guess);
	
	test_cache.reset();

}

TEST_CASE("Del", "[cache]") {
	SECTION("Del works on cached values") {
		//create objects
		bool pass = true;
		std::string keystr = "";
		const unsigned NUM_OBJ = 10; 
		unsigned array_size = 0;
		for (unsigned i = 0; i < NUM_OBJ; i++) {
			array_size = 2*i + 2;
			char *test_array = new char[array_size];
			fill_array(test_array, array_size);
			Cache::val_type test_value {test_array, array_size};
			keystr = std::to_string(array_size);
			test_cache.set(keystr, test_value);
			delete[] test_value.data_;
		}
		
		for (unsigned i = 0; i < NUM_OBJ; i++) {
			array_size = 2*i + 2;
			keystr = std::to_string(array_size);
			REQUIRE(test_cache.del(keystr));
			Cache::val_type check_value = test_cache.get(keystr);
			if(!(check_value.data_ == nullptr)) {
				pass = false;
			}
			delete[] check_value.data_;
		}
		
		REQUIRE(pass);
		test_cache.reset();
	}
	SECTION("Del returns false on key not in cache") {
		REQUIRE(!test_cache.del("foo"));
		test_cache.reset();
	}
	
	test_cache.reset();

}

TEST_CASE("Space used", "[cache]") {
  //create objects
  Cache::size_type space_guess = 0;
  std::string keystr = "";
  const unsigned NUM_OBJ = 10; 
  unsigned array_size = 0;
  
  for (unsigned i = 0; i < NUM_OBJ; i++) {
    array_size = 2*i + 2;
    char *test_array = new char[array_size];
    fill_array(test_array, array_size);
    Cache::val_type test_value {test_array, array_size};
	keystr = std::to_string(array_size);
    test_cache.set(keystr, test_value);
	space_guess += array_size;
	REQUIRE(test_cache.space_used() == space_guess);
	delete[] test_value.data_;
	}
	
	for (unsigned i = 0; i < NUM_OBJ; i++) {
    array_size = 2*i + 2;
    keystr = std::to_string(array_size);
    test_cache.del(keystr);
	space_guess -= array_size;
	REQUIRE(test_cache.space_used() == space_guess);
	}
	
	test_cache.reset();
}

TEST_CASE("Reset", "[cache]") {
	SECTION("Reset works on nonempty cache") {
		//create objects
		std::string keystr = "";
		const unsigned NUM_OBJ = 10; 
		unsigned array_size = 0;
		for (unsigned i = 0; i < NUM_OBJ; i++) {
			array_size = 2*i + 2;
			char *test_array = new char[array_size];
			fill_array(test_array, array_size);
			Cache::val_type test_value {test_array, array_size};
			keystr = std::to_string(array_size);
			test_cache.set(keystr, test_value);
			delete[] test_value.data_;
		}
		
		REQUIRE(test_cache.reset());
		REQUIRE(test_cache.hit_rate() == 0);
		REQUIRE(test_cache.space_used() == 0);
		
		for (unsigned i = 0; i < NUM_OBJ; i++) {
			array_size = 2*i + 2;
			keystr = std::to_string(array_size);
			Cache::val_type check_value = test_cache.get(keystr);
			REQUIRE(check_value.data_ == nullptr);
			delete[] check_value.data_;
		}
		
		test_cache.reset();
	}
	
	SECTION("Reset works on empty cache") {
				
		REQUIRE(test_cache.reset());
		REQUIRE(test_cache.hit_rate() == 0);
		REQUIRE(test_cache.space_used() == 0);
		
		test_cache.reset();
	}
}

TEST_CASE("Hashing", "[cache]") {

	//create objects, test set
	const unsigned NUM_OBJ = 100; 
	unsigned array_size = 0;
	std::string keystr = "";
	
	auto ti = std::chrono::high_resolution_clock::now();
	
	for (unsigned i = 0; i < NUM_OBJ; i++) {
		array_size = 2*i + 2;
		char *test_array = new char[array_size];
		fill_array(test_array, array_size);
		Cache::val_type test_value {test_array, array_size};
		keystr = std::to_string(array_size);
		
		fast_cache.set(keystr, test_value);
		
		delete[] test_value.data_;
	}
	
	auto tf = std::chrono::high_resolution_clock::now();
	
	auto t_fast = std::chrono::duration_cast<std::chrono::microseconds>(tf - ti).count();
	
	ti = std::chrono::high_resolution_clock::now();
	
	for (unsigned i = 0; i < NUM_OBJ; i++) {
		array_size = 2*i + 2;
		char *test_array = new char[array_size];
		fill_array(test_array, array_size);
		Cache::val_type test_value {test_array, array_size};
		keystr = std::to_string(array_size);
		
		slow_cache.set(keystr, test_value);
		
		delete[] test_value.data_;
	}
	
	tf = std::chrono::high_resolution_clock::now();
	
	auto t_slow = std::chrono::duration_cast<std::chrono::microseconds>(tf - ti).count();
	
	REQUIRE(t_slow > 10 * t_fast);
	
	fast_cache.reset();
	slow_cache.reset();
}
