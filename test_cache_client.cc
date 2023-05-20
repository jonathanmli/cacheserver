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
    test_array[i] = 'b';
  }
  test_array[array_size-1] = '\0';
}

//make the test objects


Cache test_cache {"127.0.0.1","8555"};

TEST_CASE("Empty get","[cache]") {

  test_cache.reset();
  
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
    delete[] test_value.data_;
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

    delete[] check_value.data_;
    delete[] test_value.data_;
    test_cache.reset();
  }

  SECTION("Old value can be replaced") {
    keystr = "a";
    array_size = 7;
    test_array = new char[array_size];//here

    fill_array(test_array, array_size);
    Cache::val_type test_value {test_array, array_size};
    test_cache.set(keystr, test_value);

    array_size = 10;
    char* test_array2 = new char[array_size];
    fill_array(test_array2, array_size);
    Cache::val_type test_value2 {test_array2, array_size};
    bool res = test_cache.set(keystr, test_value2);
    Cache::val_type check_value2 = test_cache.get(keystr); //here
    REQUIRE((res or (check_value2.data_ == nullptr)));

    delete[] test_value2.data_;
    delete[] check_value2.data_;
    delete[] test_value.data_;
    test_cache.reset();
  }
  
  test_cache.reset();
  
}

TEST_CASE("Basic get", "[cache]") {
	
  unsigned array_size = 15;
  char *test_array = new char[array_size];
  std::string keystr = "";
  fill_array(test_array,array_size);
  Cache::val_type test_value {test_array, array_size};
  test_cache.set("a",test_value);
	
  SECTION("get retrieves the correct value") {
    Cache::val_type check_value = test_cache.get("a");
		
    REQUIRE(check_value.size_ == array_size);
    for(unsigned i = 0; i < array_size-1; i++) {
      REQUIRE(check_value.data_[1] == test_value.data_[1]);
    }

    delete[] check_value.data_;
    delete[] test_value.data_;
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
  bool res = true;
	
  SECTION("set performs a deep copy") {
    Cache::val_type check_value = test_cache.get("a");
    REQUIRE(check_value.size_ == array_size);
    for(unsigned i = 0; i < array_size; i++) {
      test_array[i] += 1;
    }
    for(unsigned i = 0; i < array_size; i++) {
      res=((check_value.data_[i] != test_value.data_[i]) and res);
    }
		
    REQUIRE(res);
    delete[] check_value.data_;
		
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
  delete[] test_value.data_;
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
		Cache::val_type check_value = test_cache.get(keystr);
		if(i % 2 == 0) {hit_guess += 1;}
		else {miss_guess += 1;}
		hit_rate_guess = (double)hit_guess / (hit_guess + miss_guess);
		delete[] check_value.data_;
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

TEST_CASE("Race Conditions", "[cache]") {

	// test_cache.reset();

	const unsigned nreq = 10000;
	const unsigned array_size = 2;

	SECTION("Add many small items") {

		test_cache.reset();

		//create objects
		std::string keystr = "";
		for (unsigned i = 0; i < nreq; i++) {
			char *test_array = new char[array_size];
			fill_array(test_array, array_size);
			Cache::val_type test_value {test_array, array_size};
			keystr = std::to_string(i);
			test_cache.set(keystr, test_value);
			// if (keystr == "0"){
			// 	std::cout << test_array << std::endl;
			// 	Cache::val_type check_value = test_cache.get(keystr);
			// 	std::cout << keystr << std::endl;
			// 	REQUIRE(check_value.data_ != nullptr);
			// 	delete[] check_value.data_;
			// }
			
			delete[] test_value.data_;
		}
		REQUIRE(test_cache.space_used() == 2*nreq);

	}

	SECTION("Get many small items") {
		//get objects
		std::string keystr = "";
		for (unsigned i = 0; i < nreq; i++) {
			keystr = std::to_string(i);
			Cache::val_type check_value = test_cache.get(keystr);
			// std::cout << keystr << std::endl;
			REQUIRE(check_value.data_ != nullptr);
			delete[] check_value.data_;
		}
		REQUIRE(test_cache.space_used() == 2*nreq);
		REQUIRE(test_cache.hit_rate() == 1.0);
	}

	SECTION("Delete many small items") {
		//get objects
		std::string keystr = "";
		for (unsigned i = 0; i < nreq; i++) {
			keystr = std::to_string(i);
			REQUIRE(test_cache.del(keystr));
		}
		REQUIRE(test_cache.space_used() == 0);
		REQUIRE(test_cache.hit_rate() == 1.0);

		test_cache.reset();
	}

	
}
