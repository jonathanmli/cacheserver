#define CATCH_CONFIG_MAIN
#include "workload.hh"
#include <chrono>
#include <iostream>
#include <functional>
#include <numeric>
#include <algorithm>
#include <vector>
#include "catch.hpp"
#include "cache.hh"

Cache::size_type maxmem = 1000000;
Cache test_cache {maxmem,0.75,nullptr,std::hash<key_type>()};

//parses and sends a workload request to the cache. returns true if successful
bool send_request_to_cache(Cache& c, workload::request r) {
  key_type key{r.key};
  Cache::val_type val {r.value,sizeof(r.value)};
  //c.space_used();
  if(r.type == "get") {
    Cache::val_type ret { c.get(key)};
  	auto out = ret.data_ != nullptr;
  	delete[] ret.data_;
	return out;
  }
  if(r.type == "set") {return c.set(key,val);}
  if(r.type == "del") {return c.del(key);}
  return false;
}


//don't implement these yet
TEST_CASE("Set Ratio","[work]") {
	const auto ratio = 0.02;
	double setcount = 0;
	double nreq = 10000;
	workload work {0};
	work.set_parameters(50,2,48);
	work.gen_reqs(nreq);
	auto reqs = work.get_reqs();
	for (auto r : reqs) {
	  if(r.type=="set"){setcount+=1;}
	}
	REQUIRE((setcount/nreq)==Approx(ratio).epsilon(0.1));
}


TEST_CASE("Del Ratio", "[work]") {
  const auto ratio = 0.02;
	double setcount = 0;
	double nreq = 10000;
	workload work {0};
	work.set_parameters(50,48,2);
	work.gen_reqs(nreq);
	auto reqs = work.get_reqs();
	for (auto r : reqs) {
	  if(r.type=="del"){setcount+=1;}
	}
	REQUIRE((setcount/nreq)==Approx(ratio).epsilon(0.1));
}

TEST_CASE("Get Ratio", "[work]") {
  const auto ratio = 0.02;
	double setcount = 0;
	double nreq = 10000;
	workload work {0};
	work.set_parameters(2,50,48);
	work.gen_reqs(nreq);
	auto reqs = work.get_reqs();
	for (auto r : reqs) {
	  if(r.type=="get"){setcount+=1;}
	}
	REQUIRE((setcount/nreq)==Approx(ratio).epsilon(0.1));

}


TEST_CASE("Hit Rate ", "[work, test_cache]") {
	
	test_cache.reset();

	auto nreq = 100000;
	// auto hits 

	//scoped in order to deconstruct workload after use
	if(true) {
		//warm up cache first
		workload wk {nreq/2};
		auto rs = wk.get_reqs();
		for (auto r : rs) {
			send_request_to_cache(test_cache, r);
		}
	}
	

    //now the hit rate counting
	workload work {nreq};

	std::cout << "I made a workload\n";

	auto successes = 0;
	auto reqs = work.get_reqs();
	for (auto r : reqs) {
		if(send_request_to_cache(test_cache, r)) {
			successes++;
		}
	}

	
	
	REQUIRE((static_cast<float>(successes))/(static_cast<float> (nreq)) == Approx(0.8).epsilon(0.1));

	test_cache.reset();
}

TEST_CASE("Key Invalidation ", "[work, test_cache]") {
	
	auto nreq = 100000;
	// auto hits 

	//warm up cache?

	double dels = 0;
	double invals = 0;
	double inval_ratio = 0.1;
	
	workload work {nreq};

	auto reqs = work.get_reqs();
	bool rout = false;
	for (auto r : reqs) {
		rout = send_request_to_cache(test_cache, r);
		if (r.type == "del") {
		  dels ++;
		  if(rout) {invals++;}
		}
	}
	
	REQUIRE((invals/dels) == Approx(inval_ratio).epsilon(0.05));

	test_cache.reset();
}

