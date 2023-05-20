#define CATCH_CONFIG_MAIN
#include "lru_evictor.hh"
#include "catch.hpp"
//#include <catch2/catch.hpp>

using Lru_evictor = Lru_evictor;

Lru_evictor test_lru {};

TEST_CASE("Lru Eviction", "[Lru_evictor]") { 
  
  SECTION("Returns empty string when empty") {
    REQUIRE(test_lru.evict() == "");
  }

  SECTION("Can touch keys") {
    test_lru.touch_key("a");
    test_lru.touch_key("b");
    test_lru.touch_key("c");
    test_lru.touch_key("a");
    test_lru.touch_key("c");
  }

  SECTION("Evicts last items touched") {
    REQUIRE(test_lru.evict() == "b");
    REQUIRE(test_lru.evict() == "a");
    REQUIRE(test_lru.evict() == "c");
    REQUIRE(test_lru.evict() == "");
  }
}


