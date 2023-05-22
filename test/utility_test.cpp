#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "utility.hpp"

TEST_CASE("FindSubStr") {
    const char* str = "Hello1233321Worl00World";

    REQUIRE(FindSubStr((uint8_t*)str, strlen(str), (uint8_t*)"Hello", 5) == (uint8_t*)str);
    REQUIRE(FindSubStr((uint8_t*)str, strlen(str), (uint8_t*)"World", 5) == (uint8_t*)str + 18);
    REQUIRE(FindSubStr((uint8_t*)str, strlen(str), (uint8_t*)"Worl", 4) ==  (uint8_t*)str + 12);
    REQUIRE(FindSubStr((uint8_t*)str, strlen(str), (uint8_t*)"Worldl", 6) == nullptr);
}